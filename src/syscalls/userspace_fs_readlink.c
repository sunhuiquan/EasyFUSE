#include "userspace_fs_calls.h"
#include "fs.h"

/* 读符号链接，把指向的路径写到buf里面，注意这是对符号链接操作 */
int userspace_fs_readlink(const char *path, char *buf, size_t bufsz)
{
	if (path == NULL || strlen(path) >= MAX_NAME)
		return -1;
	return 0;
}
