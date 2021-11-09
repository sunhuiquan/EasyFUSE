#include "fuse_fs_calls.h"
#include "fs.h"
#include "disk.h"
#include <stdio.h>
#include <string.h>

#define FUSE_FILE S_IFREG // 原来就是通过对应掩码取与检测的，所以倒过来就是这个值
#define FUSE_DIR S_IFDIR

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
int fuse_fs_stat(const char *path, struct stat *sbuf)
{
	memset(sbuf, 0, sizeof(struct stat));
	printf("pathname: %s\n", path);

	sbuf->st_ino = superblock.data_block_startno;
	sbuf->st_size = 1234;
	sbuf->st_mode = FUSE_DIR | 0666;

	return 0;
}