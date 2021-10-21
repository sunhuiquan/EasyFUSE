#define FUSE_USE_VERSION 27
#include <fuse.h>
// 如果没有在引用fuse.h之前定义版本，那么自动是21版本，有些不同
// 注意一定是在引用fuse.h之前define，因为之后改没用，因为已经设置完了

#include <stdlib.h>

/* 实现接口的函数声明 */
static int ufs_getattr(const char *path, struct stat *stbuf);

/* 对libfuse库接口的实现 */
static struct fuse_operations u_operation = {.getattr = fuse_fs_getattr};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &u_operation, NULL);
}

static int fuse_fs_getattr(const char *path, struct stat *stbuf)
{
	return -1;
}
