#include "userspace_fs_calls.h"
#include "fs.h"
#include "disk.h"
#include "inode_cache.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
// to do 没有加时间属性和设备属性
int userspace_fs_stat(const char *path, struct stat *sbuf)
{
	char basename[MAX_NAME];
	struct inode *pinode;
	if ((pinode = find_path_inode(path, basename)) == NULL)
		return -1;

	sbuf->st_ino = pinode->inum;
	sbuf->st_size = pinode->dinode.size;
	sbuf->st_dev = pinode->dev;
	sbuf->st_nlink = pinode->dinode.nlink;

	if (pinode->dinode.type == FUSE_DIR)
		sbuf->st_mode = S_IFDIR | 0777;
	else if (pinode->dinode.type == FUSE_FILE)
		sbuf->st_mode = S_IFREG | 0777;
	else if (pinode->dinode.type == FUSE_LNK)
		sbuf->st_mode = S_IFLNK | 0777;
	else
		return -1;

	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;
	return 0;
}