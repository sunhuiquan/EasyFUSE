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

	if (create(path, type) == NULL)
	{
		if (out_transaction() == -1)
			return -1;
		return -1;
	}

	if (out_transaction() == -1)
		return -1;
	return 0;
}

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type)
{
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return NULL;

	if (inode_lock(dir_pinode) == -1) // 对dp加锁
		return NULL;

	// 已存在该 name 的文件，直接返回这个（类似 O_CREATE 不含 O_EXEC 的语义）
	if ((pinode = dir_find(dir_pinode, basename)) != NULL) // dir_find获取的inode是没有加锁过的
	{
		if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
			return NULL;

		if (inode_lock(pinode) == -1) // 为了保证如果成功返回是有锁的，保持一致
			return NULL;

		if (pinode->dinode.type == type)
			return pinode;

		if (inode_unlock_then_reduce_ref(pinode) == -1)
			return NULL;
		return NULL;
	}

	// 这个name的文件不存在，所以新建一个 inode 表示这个文件，然后在文件所在目录里面新建一个目录项，指向这个 inode

	// 分配一个新的inode(一定是icache不命中)，内部通过iget得到缓存中的inode结构(此时valid为0，
	// 实际内容未加载到内存中)，然后未持有锁，且引用计数为1
	if ((pinode = inode_allocate(type)) == NULL) // 返回的是没有加锁的inode指针
		return NULL;

	if (inode_lock(pinode) == -1) // 加锁
		return NULL;
	pinode->dinode.nlink = 1;
	if (inode_update(pinode) == -1)
		return NULL;

	if (type == FILE_DIR) // 添加 . 和 .. 目录项
	{
		++dir_pinode->dinode.nlink; // .. 指向上一级目录
		// No ip->nlink++ for ".": avoid cyclic ref count.??
		if (inode_update(dir_pinode) == -1)
			return NULL;

		if (add_dirent_entry(pinode, ".", pinode->inum) == -1 || add_dirent_entry(pinode, "..", dir_pinode->inum) == -1)
			return NULL;
	}
	if (add_dirent_entry(dir_pinode, basename, pinode->inum) == -1)
		return NULL;

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
		return NULL;
	return pinode;
}
