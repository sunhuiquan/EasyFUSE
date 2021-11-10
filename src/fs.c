#include "fs.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>

/* 返回路径文件所在的目录(不是文件本身，是它所在的目录)的 inode，通过iget，返回未加锁，且已增加了引用计数
 *
 * 注意const char*的path，因为我们只改变副本形参指针的指向，与外面的实参无关。
 */
struct inode *find_dir_inode(const char *path, char *name)
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

// 返回路径对应文件的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_path_inode(const char *path, char *name)
{
	struct inode *pinode, *next;

	pinode = iget(ROOT_INODE); // iget返回的是未加锁的inode指针
	while ((path = current_dir_name(path, name)) != NULL)
	{
		if (inode_lock(pinode) == -1)
			return NULL;

		if (*name == '\0')
		{
			if (inode_unlock(pinode) == -1) // 解锁
				return NULL;
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
}

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => '\0'
 * 			"/a/"  => '\0'  name"a"
 * 			"/a/b" => path:"b" name:"a"
 * (多余的 '/' 不影响结果，最后如果是 '/' 会忽略，不会当成路径)
 */
const char *current_dir_name(const char *path, char *name)
{
	const char *s;
	int len;

	while (*path == '/')
		path++;
	if (*path == 0) // 空字符串错误
		return NULL;
	s = path;
	while (*path != '/' && *path != '\0')
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
int add_dirent_entry(struct inode *pdi, char *name, uint inum)
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
