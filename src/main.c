#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>

#include "userspace_fs_calls.h" // 所有FUSE系统调用的声明

/* 使用Debug模式构建，编译无优化，相当于-g，使得能被gdb调试；
 * 关于运行错误检测，开发的时候开启-d选项，这样libfuse的接口实现的函数返回-1的时候，程序会输出
 * 信息，告诉我们哪里出了错误，用于开发检测，不过这个错误是非强制退出的，不处理则会忽略并继续。
 *
 * 发行版本使用Release模式构建，无测试信息，高级别编译优化；
 * 运行时用户不开启-d选项，作为守护进程后台运行，忽略错误并继续。
 */

/* 对libfuse库接口的实现，我们在我们实现的FUSE系统调用中，对-1情况直接panic终止程序，报告错误 */
static struct fuse_operations u_operation = {
	.init = userspace_fs_init,
	.getattr = userspace_fs_stat,
	.readdir = userspace_fs_readdir,
	.mkdir = userspace_fs_mkdir,
	.unlink = userspace_fs_unlink,
	.rmdir = userspace_fs_rmdir,
	// .open = userspace_fs_open
};

/* 注意fuse_main注册的u_operation里面的函数返回值对libfuse的作用的，例如getattr返回-ENOENT代表无此文件，
 * 通知了libfuse做相应的操作。
 *
 * 但是我们的实现里面是glibc风格，我们返回的是-1代表错误，-errno的值并没有返回，返回-1在libfuse看起来是
 * operation no permitted的错误，但实际上这是我们用-1表示通用的错误，具体错误原因并不是这个。
 *
 * 所以对于某些错误，libfuse需要使用的，我们要给与对应的-errno返回值，虽然我们的实现有点问题，都用-errno返回
 * 也许更好，但是由于不想改整体代码的原因，我们仅在libfuse必需要用的情况的确返回对应的-errno值。
 */
int main(int argc, char *argv[])
{
	int ret = fuse_main(argc, argv, &u_operation, NULL);
	if (ret != 0)
		printf("fuse_main failed：%d\n", ret);
	return ret;
}
