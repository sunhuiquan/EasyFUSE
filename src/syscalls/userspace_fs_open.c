/* 实现FUSE系统调用 */

#include "userspace_fs_calls.h"
#include "log.h"
#include "fs.h"
#include <string.h>

#include <util.h>

/* 实现 libfuse open 系统调用 */
int userspace_fs_open(const char *path, struct fuse_file_info *fi)
{
	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;
	pr_open_flags(fi);

	/* fi->flags 是对应的 open 使用参数，但我们不需要处理，libfuse会根据这个 flags 自动调用
	 * 我们实现过了的 userspace_fs_calls 函数，不用自己处理。
	 *
	 * 对于 O_RDONLY, O_WRONLY, O_RDWR libfuse 会自动在调用堆用的 userspace_fs_write 和
	 * userspace_fs_read 实现时检查，不需要我们处理；
	 *
	 * 同样 O_APPEND 也是自动处理，会在调用 userspace_fs_write 时传递文件末尾的 offset 值；
	 *
	 * 对于 O_CREAT 和 O_EXCL 会先 userspace_fs_getattr 获取文件是否存在，再根据它们的语义调用
	 * userspace_fs_create 调用；如果查找不到文件，那么会 userspace_fs_create 创建并直接打开文件，
	 * 不会调用 userspace_fs_open 了，如果查找不到，那么会进入 userspace_fs_open 而且 flags 此时
	 * 以及去掉了 O_CREATE 标志；另外如果还有 O_EXCL 那么如果文件已存在会导致错误返回。
	 */

	return 0;
}
