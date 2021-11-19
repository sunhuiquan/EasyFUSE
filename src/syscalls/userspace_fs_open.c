/* 实现FUSE系统调用 */

#include "userspace_fs_calls.h"
#include "log.h"
#include "fs.h"
#include <string.h>

#include <stdio.h>
#include <util.h>

/* 实现 libfuse open 系统调用 */
int userspace_fs_open(const char *path, struct fuse_file_info *fi)
{
	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;
	pr_open_flags(fi);

	/* 对于 O_RDONLY, O_WRONLY, O_RDWR libfuse 会自动在调用堆用的 userspace_fs_write 和
	 * userspace_fs_read 实现时检查，不需要我们处理；
	 * 
	 * 同样 O_APPEND 也是自动处理，会在调用 userspace_fs_write 时传递文件末尾的 offset 值；
	 *
	 * to do
	 * 
	 */

	// if (fi->flags & O_CREAT)
	// {
	// 	if (fi->flags & O_EXCL)
	// 	{
	// 	}
	// }
	// else if (fi->flags & O_EXCL)
	// 	return -1;

	return 0;
}
