#include "userspace_fs_calls.h"
#include "fs.h"

/* 实现 libfuse create 系统调用，可用于创建普通文件 */
int userspace_fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	return 0;
}
