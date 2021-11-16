#include "userspace_fs_calls.h"
#include "inode_cache.h"
#include "fs.h"
#include <string.h>

/* 读符号链接，把指向的路径写到buf里面，注意这是对符号链接操作 */
int userspace_fs_readlink(const char *path, char *buf, size_t bufsz)
{
	struct inode *pinode;
	char temp[MAX_NAME];
	uint sz;

	if (path == NULL || strlen(path) >= MAX_PATH)
		return -1;

	if ((pinode = find_path_inode(path, temp)) == NULL)
		return -1;
	if (inode_lock(pinode) == -1)
	{
		inode_reduce_ref(pinode);
		return -1;
	}

	sz = (MAX_PATH <= bufsz) ? MAX_PATH : bufsz;
	if (readinode(pinode, buf, 0, sz) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;
	return 0;
}
