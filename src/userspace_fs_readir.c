#include "userspace_fs_calls.h"
#include "fs.h"
#include "inode_cache.h"

/* 读取path目录的目录项，要检测该path必须是一个目录文件，参数里面的buf和filler用于添加目录项，fih和off_set用不到 */
static int userspace_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
								off_t offset, struct fuse_file_info *fi)
{
	char basename[MAX_NAME];
	struct inode *pinode;
	if ((pinode = find_path_inode(path, basename)) == NULL)
		return -1;

	if (inode_lock(pinode) == -1) // 加锁，并内置inode_load
		return -1;
	if (pinode->dinode.type != FILE_DIR) // 该path必须是一个目录文件
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	// to do
	
	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;
	return 0;
}