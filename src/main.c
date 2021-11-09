#define FUSE_USE_VERSION 27
#include <fuse.h>
#include <stdlib.h>

#include "fuse_fs_calls.h" // 所有FS系统调用的声明

/* 对libfuse库接口的实现 */
static struct fuse_operations u_operation = {.getattr = fuse_fs_stat};

int main(int argc, char *argv[])
{
	int ret = fuse_main(argc, argv, &u_operation, NULL);
	if (ret != 0)
		printf("fuse_main failed：%d\n", ret);
	return ret;
}
