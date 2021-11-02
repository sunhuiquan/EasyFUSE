#include "fuse_fs_open.h"
#include "inode_cache.h"
#include "block_cache.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <pthread.h>

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type)
{
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return -1;

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
		return -1;

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
			return -1;
	}

	if (add_dirent_entry(dir_pinode, basename, pinode->inum) == -1)
		return -1;

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
		return NULL;

	return pinode;
}

// 返回路径文件所在的目录的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_dir_inode(char *path, char *name)
{
	struct inode *pinode, *next;

	pinode = iget(ROOT_INODE); // iget返回的是未加锁的inode指针
	while ((path = current_dir_name(path, name)) != NULL)
	{
		if (inode_lock(pinode) == -1)
			return NULL;
		// name 是当前层的中间name
		// 路径中间的一个name不是目录文件，错误
		if (pinode->dinode.type != FILE_DIR)
		{
			if (inode_unlock_then_reduce_ref(pinode) == -1)
				return NULL;
			return NULL;
		}

		// 在name为最后一个文件名的时候进入，在下一个 dir_find 前 return 了，返回的是所在目录的inode，如果不中途返回那么 dir_find 这个 name 就是返回该文件的 inode
		if (*path == '\0')
		{
			if (inode_unlock(pinode) == -1) // 解锁
				return NULL;
			// 这里返回的inode*要被使用，可不要减引用，不过要解锁
			return pinode;
		}

		// 查找目录项，通过已有的该目录下的 inode 获取对应 name 的 inode
		if ((next = dir_find(pinode, name)) == NULL)
		{
			if (inode_unlock_then_reduce_ref(pinode) == -1)
				return NULL;
			return NULL;
		}

		if (inode_unlock_then_reduce_ref(pinode) == -1)
			return NULL;
		pinode = next;
	}
	if (inode_reduce_ref(pinode) == -1) // 减引用
		return NULL;
	return NULL;
}

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => NULL
 * 			"/a/"  => NULL  name"a"
 * 			"/a/b" => path:"b" name:"b"
 * (多余的 '/' 不影响结果，最后如果是 '/' 会忽略，不会当成路径)
 */
char *current_dir_name(char *path, char *name)
{
	char *s;
	int len;

	while (*path == '/')
		path++;
	if (*path == 0) // 空字符串错误
		return NULL;
	s = path;
	while (*path != '/' && *path != 0)
		path++;
	len = path - s;
	if (len >= MAX_NAME)
		memmove(name, s, MAX_NAME);
	else
	{
		memmove(name, s, len);
		name[len] = 0;
	}
	while (*path == '/')
		path++;
	return path;
}

/*!
 * 通过目录 inode 查找该目录下对应 name 的 inode 并返回，通过iget，返回未加锁，且已增加了引用计数。
 * (内存中调用iget就说明有新的对象指向了这个inode缓存，调用iget就会导致引用计数+1)
 *
 * \param pdi A pointer to current directory inode.
 */
struct inode *dir_find(struct inode *pdi, char *name)
{
	struct dirent dirent;

	if (pdi->dinode.type != FILE_DIR)
		return NULL;

	uint sz = pdi->dinode.size;
	for (uint off = 0; off < sz; off += sizeof(struct dirent))
	{
		if (readinode(pdi, &dirent, off, sizeof(struct dirent)) != sizeof(struct dirent))
			return NULL;
		if (dirent.inum != 0 && !strncmp(dirent.name, name, MAX_NAME))
			return iget(dirent.inum);
	}
	return NULL;
}

// 将name和inum组合成一条dirent结构，写入pdi这个目标inode结构，注意pdi是持有锁的
int add_dirent_entry(struct inode *pdi, const char *name, uint inum)
{
	struct dirent dir;
	struct inode *pi;

	if ((pi = dir_find(pdi, name)) != NULL)
	{
		if (inode_reduce_ref(pi) == -1) // 没加锁，所以只减引用计数
			return -1;
		return -1;
	}
	// pi == NULL 代表该目录里面没有这个名字的目录项

	int off = 0;
	for (; off < pdi->dinode.size; off += sizeof(struct dirent))
	{
		if (readinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
			return -1;
		if (dir.inum == 0)
			break;
	}

	strncpy(dir.name, name, MAX_NAME);
	dir.inum = inum;
	if (writeinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
		return -1;

	return 0;
}
