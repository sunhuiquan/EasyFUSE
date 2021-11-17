#include "userspace_fs_calls.h"
#include "fs.h"
#include "log.h"

/* 实现 libfuse write 系统调用 */
int userspace_fs_write(const char *path, const char *buf, size_t count,
					   off_t offset, struct fuse_file_info *fi)
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
}