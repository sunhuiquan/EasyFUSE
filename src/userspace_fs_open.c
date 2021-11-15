/* 实现FUSE系统调用 */

#include "userspace_fs_calls.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include "fs.h"

/* wraper */
int wraper(const char *path, short type)
{
	if (in_transaction() == -1)
		return -1;

	if (inner_create(path, type) == NULL)
	{
		if (out_transaction() == -1)
			return -1;
		return -1;
	}

	if (out_transaction() == -1)
		return -1;
	return 0;
}
