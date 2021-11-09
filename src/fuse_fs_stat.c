#include "fuse_fs_calls.h"
#include "fs.h"
#include <stdio.h>
#include <string.h>

#define REG_FILE S_IFREG // 原来就是通过对应掩码取与检测的，所以倒过来就是这个值

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
int fuse_fs_stat(const char *path, struct stat *sbuf)
{
	memset(sbuf, 0, sizeof(struct stat));
	printf("pathname: %s\n", path);

	sbuf->st_ino = 1234;
	sbuf->st_size = 5678;
	sbuf->st_mode = REG_FILE | 0777;

	return 0;
}