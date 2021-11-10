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
static struct fuse_operations u_operation = {.getattr = userspace_fs_stat, .init = userspace_fs_init};

int main(int argc, char *argv[])
{
	int ret = fuse_main(argc, argv, &u_operation, NULL);
	if (ret != 0)
		printf("fuse_main failed：%d\n", ret);
	return ret;
}
