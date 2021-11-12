/* 实现FUSE系统调用 */

#include "userspace_fs_calls.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include "fs.h"

/* wraper */
int wraper(char *path, ushort type)
{
	if (in_transaction() == -1)
		return -1;

	if (userspace_fs_create(path, type) == NULL)
	{
		if (out_transaction() == -1)
			return -1;
		return -1;
	}

	if (out_transaction() == -1)
		return -1;
	return 0;
}
