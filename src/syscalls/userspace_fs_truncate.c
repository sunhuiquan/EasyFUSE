#include "userspace_fs_calls.h"
#include "fs.h"
#include "log.h"
#include <string.h>

/* 实现 libfuse truncate 系统调用 */
int userspace_fs_truncate(const char *path, off_t offset)
{
	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	// to do

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;

	return 0;
}