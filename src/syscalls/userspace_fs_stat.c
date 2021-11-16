#include "userspace_fs_calls.h"
#include "fs.h"
#include "disk.h"
#include "inode_cache.h"
#include "util.h"
#include <errno.h>
#include <string.h>

/* 获取文件属性，返回-ENOENT代表无此文件，注意要直接返回给libfuse接口用于通知 */
// to do 没有加时间属性和设备属性
// to do 增加权限标志，具体的权限检测是linux做的，我们只是设置标志位
int userspace_fs_stat(const char *path, struct stat *sbuf)
{
	char basename[MAX_NAME];
	struct inode *pinode;

	if (path == NULL || strlen(path) >= MAX_NAME)
		return -1;

	if ((pinode = find_path_inode(path, basename)) == NULL)
		return -ENOENT; // 无此文件，用于通知ligfuse LOOK UP找不到文件

	// 给pinode加锁，虽然这里是读，但是如果中间被修改，那么我们读的这个pinode信息就是不一致的，可能导致严重错误
	// 同时这也保证了如果valid 0那么就进行inode_load操作
	if (inode_lock(pinode) == -1)
		return -1;
	sbuf->st_ino = pinode->inum;
	sbuf->st_size = pinode->dinode.size;
	sbuf->st_dev = pinode->dev;
	sbuf->st_nlink = pinode->dinode.nlink;

	if (pinode->dinode.type == FILE_DIR)
		sbuf->st_mode = S_IFDIR | 0777;
	else if (pinode->dinode.type == FILE_REG)
		sbuf->st_mode = S_IFREG | 0777;
	else if (pinode->dinode.type == FILE_LNK)
		sbuf->st_mode = S_IFLNK | 0777;
	else
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;
	return 0;
}