#include "userspace_fs_calls.h"
#include "fs.h"
#include "inode_cache.h"
#include <string.h>

/* 实现 libfuse read 系统调用 */
int userspace_fs_read(const char *path, char *buf, size_t bufsz,
					  off_t offset, struct fuse_file_info *fi)
{
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
