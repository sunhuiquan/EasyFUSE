#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>

#include "userspace_fs_calls.h" // 所有FUSE系统调用的声明

/* 对libfuse库接口的实现 */
static struct fuse_operations u_operation = {.getattr = userspace_fs_stat, .init = userspace_fs_init};

int main(int argc, char *argv[])
{
	int ret = fuse_main(argc, argv, &u_operation, NULL);
	if (ret != 0)
		printf("fuse_main failed：%d\n", ret);
	return ret;
}
