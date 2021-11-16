#include "userspace_fs_calls.h"
#include "log.h"
#include "inode_cache.h"
#include <string.h>

/* 创建硬链接，注意不能给目录创建硬链接 */
int userspace_fs_link(const char *oldpath, const char *newpath)
{
	if (oldpath == NULL || newpath == NULL)
		return -1;
	if (strlen(oldpath) >= MAX_NAME || strlen(newpath) >= MAX_NAME)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	if (inner_link(oldpath, newpath) == -1)
		return -1;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}
