#include "userspace_fs_calls.h"
#include "fs.h"
#include "log.h"
#include <string.h>

#include <stdio.h>

/* 实现 libfuse write 系统调用 */
int userspace_fs_write(const char *path, const char *buf, size_t count,
					   off_t offset, struct fuse_file_info *fi)
{
	/* 注意fi->flags为0外面这里使用不了，这不会影响外面的实现，因为libfuse在外面已经检测过了
	 * 只有O_WRONLY或O_RDWR才能调用write，不需要我们自己在write中实现对flags的检查
	 * 另外知道如果指定了O_OFFSET，那么得来的offset是文件末尾，不需要我们自己在open里面处理。
	 */
	printf("offset: %ld\n", (long)offset);

	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	// to do fi打开文件标志的权限检测？？
	int writen;
	if ((writen = inner_write(path, buf, count, offset)) == -1)
	{
		out_transaction();
		return writen;
	}

	if (out_transaction() == -1) // 离开事务
		return -1;
	return writen;
}
