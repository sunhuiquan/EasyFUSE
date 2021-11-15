#include "userspace_fs_calls.h"
#include "log.h"

/* rmdir命令删除目录 */
int userspace_fs_rmdir(const char *path)
{

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	if (inner_unlink(path) == -1)
		return -1;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}