#include "fuse_fs_calls.h"
#include "fs.h"
#include <sys/stat.h>

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
int fuse_fs_stat(const char *path, struct stat *sbuf)
{
	printf("pathname: %s\n", path);
	sbuf->st_ino = 1234;
	sbuf->st_mode = 0777;
	return 0;
}