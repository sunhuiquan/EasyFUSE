#include "userspace_fs_calls.h"
#include "fs.h"
#include "log.h"
#include <string.h>

/* 实现 libfuse rename 系统调用
 *
 * 注意rename的本质是改变目录项，把一个目录数据块中的目录项，移动到另一个(也可以是同一个)目录数据块
 * 中，中间过程可以改变目录项的name，inum不变。本质上目录项就是硬链接，所以说这个rename本质就是对
 * 硬链接移动（可能改变目录项所在的目录数据块）并改名（中间改变目录项的name）。
 */
int userspace_fs_rename(const char *oldpath, const char *newpath)
{
	if (oldpath == NULL || newpath == NULL)
		return -1;
	if (strlen(oldpath) >= MAX_PATH || strlen(newpath) >= MAX_PATH)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	if (inner_rename(oldpath, newpath) == -1)
	{
		out_transaction();
		return -1;
	}

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}