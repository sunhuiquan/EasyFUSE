#include "userspace_fs_calls.h"
#include "fs.h"
#include "inode_cache.h"
#include <string.h>

#include <stdio.h>

/* 实现 libfuse read 系统调用 */
int userspace_fs_read(const char *path, char *buf, size_t bufsz,
					  off_t offset, struct fuse_file_info *fi)
{
	// 注意fi->flags为0外面这里使用不了，这不会影响外面的实现，因为libfuse在外面已经检测过了
	// 只有O_RDONLY或O_RDWR才能调用read，不需要我们自己在read中实现对flags的检查
	printf("offset: %ld\n", (long)offset);

	/* Linux 给外面的read系统调用提供了预读机制，外面FUSE的实现得到的bufsz是预读机制给的大小，
	 * 大概率是大的，这样多次read只会一次经VFS调用内部实现，提高效率；
	 *
	 * 因此这里的 offset 的值是 linux 预读机制块的倍数(4096B)，导致了这里传递来的 offset 并不是
	 * 真正要读的偏移量，而是一次预读要读的偏移量：
	 * 比如我们要读 5000次 1B，那么前 4096次调用一次 userspace_fs_read，后 904次调用一次，两次的
	 * offset分别是 0和 4096，变 5000次1B，变两次，一次 4096B，一次 904B，之后的逻辑由 glibc 的
	 * read系统调用实现，那里的offset和bufsz才是真正的offset和bufsz，完全和write的offset一致，
	 * 比如write到最后再读就会EOF这样。
	 */
	struct inode *pinode;
	char temp[MAX_NAME];
	int readn;

	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	if ((pinode = find_path_inode(path, temp)) == NULL)
		return -1;
	if (inode_lock(pinode) == -1)
	{
		inode_reduce_ref(pinode);
		return -1;
	}

	// 为了简化，我们要么全部写入返回要写入数的值，要么返回-1
	if ((readn = readinode(pinode, buf, offset, bufsz)) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;

	return readn;
}
