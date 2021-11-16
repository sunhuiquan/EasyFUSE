#include "userspace_fs_calls.h"
#include "log.h"
#include "fs.h"
#include <string.h>

/* 创建符号链接，可以为目录创建符号链接 */
int userspace_fs_symlink(const char *oldpath, const char *newpath)
{
	if (oldpath == NULL || newpath == NULL)
		return -1;
	if (strlen(oldpath) >= MAX_PATH || strlen(newpath) >= MAX_PATH)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	// 之所以可以为目录创建符号链接，是因为在要递归遍历目录的情况下，我们不递归符号链接目录即可，不会破坏树形结构。
	if (inner_symlink(oldpath, newpath) == -1)
		return -1;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}
