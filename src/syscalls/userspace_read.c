#include "userspace_fs_calls.h"
#include "fs.h"

/* 实现 libfuse read 系统调用 */
int userspace_fs_read(const char *path, char *buf, size_t bufsz,
					  off_t offset, struct fuse_file_info *fi)
{
	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	return 0;
}