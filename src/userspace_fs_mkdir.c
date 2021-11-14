#include "userspace_fs_calls.h"
#include "fs.h"
#include "inode_cache.h"
#include "log.h"
#include <string.h>

/* mkdir命令创建目录 */
int userspace_fs_mkdir(const char *path, mode_t mode)
{
	struct inode *pinode;

	if (in_transaction() == -1) // 进入事务
		return -1;

	if (strlen(path) >= MAX_NAME) // 等于会没有'\0'导致溢出
		return -1;

	mode |= S_IFDIR; // libfuse 文档写着 To obtain the correct directory type bits use mode|S_IFDIR.

	// to do??这里还没有实现mode_t权限的接口
	if ((pinode = userspace_fs_create(path, FILE_DIR)) == NULL)
		return -1;
	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}