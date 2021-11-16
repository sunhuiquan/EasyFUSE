#include "userspace_fs_calls.h"
#include "log.h"

/* 创建硬链接，注意不能给目录创建硬链接 */
int userspace_fs_link(const char *oldpath, const char *newpath)
{
	/* 不能给目录创建硬链接(Linux标准，其他一些UNIX系统也许可以，我们这个类UNIX文件系统
	 * 采取类Linux标准)，这是为了避免破坏目录的树形结构。
	 */
	return 0;
}
