#include "userspace_fs_calls.h"
#include "log.h"
#include "inode_cache.h"
#include <string.h>

/* 创建硬链接，注意不能给目录创建硬链接 */
int userspace_fs_link(const char *oldpath, const char *newpath)
{
	/* 1. 硬链接不能给目录创建硬链接(Linux标准，其他一些UNIX系统也许可以，我们这个类UNIX文件系统
	 *    采取类Linux标准)，这是为了避免破坏目录的树形结构。
	 *
	 * 2. inode是一个文件的元数据，而硬链接只是一个指向该inode的inum的一个目录项，两者完全没有关系，
	 *    所以硬链接只是一个已存在的目录的目录项，与符号链接不同根本没有inode。
	 */

	struct inode *pinode;	  // oldpath 文件的 inode
	struct inode *dir_pinode; // newpath 所在目录的 inode
	char basename[MAX_NAME];  // newpath 中的 basename
	char temp[MAX_NAME];	  // oldpath basename 用不到
	uint offset;

	if (strlen(oldpath) >= MAX_NAME || strlen(newpath) >= MAX_NAME)
		return -1;

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	if ((dir_pinode = find_dir_inode(newpath, basename)) == NULL)
		return NULL;
	if (inode_lock(dir_pinode) == -1)
	{
		inode_reduce_ref(dir_pinode);
		return NULL;
	}

	if ((pinode = find_path_inode(oldpath, temp)) == NULL)
		goto bad;
	if (inode_lock(pinode) == -1)
	{
		inode_reduce_ref(pinode);
		goto bad;
	}

	// 在新路径所在的上级目录创建目录项
	if (add_dirent_entry(dir_pinode, basename, pinode->inum) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		goto bad;
	}

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return NULL;
	}

	// 增加inode硬链接计数
	++pinode->dinode.nlink;
	if (inode_update(pinode) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return NULL;
	}
	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return NULL;

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;

bad:
	inode_unlock_then_reduce_ref(dir_pinode);
	return NULL;
}
