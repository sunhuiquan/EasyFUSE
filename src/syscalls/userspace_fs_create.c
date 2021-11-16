#include "userspace_fs_calls.h"
#include "fs.h"
#include "log.h"
#include "inode_cache.h"
#include <string.h>

/* 实现 libfuse create 系统调用，可用于创建普通文件 */
int userspace_fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	struct inode *pinode;

	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	// to do??这里还没有实现mode_t权限的接口
	if ((pinode = inner_create(path, FILE_REG)) == NULL)
		return -1;
	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}
