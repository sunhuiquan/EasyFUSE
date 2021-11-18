/* 实现FUSE系统调用 */

#include "userspace_fs_calls.h"
#include "log.h"
#include "fs.h"
#include <string.h>

#include <stdio.h>
static int a = 1;

/* 实现 libfuse open 系统调用 */
int userspace_fs_open(const char *path, struct fuse_file_info *fi)
{
	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	printf("%ld\n", (long)fi->fh);
	fi->fh = a++;
	printf("%ld\n", (long)fi->fh);
	printf("flags: %d\n", fi->flags);

	return 0;
}
