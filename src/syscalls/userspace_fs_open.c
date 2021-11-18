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
