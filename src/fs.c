#include "fs.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

static int dir_is_empty(struct inode *pi);

/* 返回路径文件所在的目录(不是文件本身，是它所在的目录)的 inode，通过iget，返回未加锁，且已增加了引用计数
 *
 * 注意const char*的path，因为我们只改变副本形参指针的指向，与外面的实参无关。
 */
struct inode *find_dir_inode(const char *path, char *name)
{
	struct inode *pinode, *next;

	pinode = iget(ROOT_INODE); // iget返回的是未加锁的inode指针
	while (1)
	{
		path = current_dir_name(path, name);
		if (*name == '\0')
			return NULL;

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
		if ((next = dir_find(pinode, name, NULL)) == NULL)
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
// to do ?? 相对路径的支持
struct inode *find_path_inode(const char *path, char *name)
{
	struct inode *pinode, *next;

	pinode = iget(ROOT_INODE); // iget返回的是未加锁的inode指针
	while (1)
	{
		path = current_dir_name(path, name);

		if (inode_lock(pinode) == -1)
			return NULL;

		if (*name == '\0')
		{
			if (inode_unlock(pinode) == -1) // 解锁
				return NULL;
			return pinode;
		}

		// 查找目录项，通过已有的该目录下的 inode 获取对应 name 的 inode
		if ((next = dir_find(pinode, name, NULL)) == NULL)
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
struct inode *dir_find(struct inode *pdi, char *name, uint *offset)
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
		{
			if (offset)
				*offset = off;
			return iget(dirent.inum);
		}
	}
	return NULL;
}

// 将name和inum组合成一条dirent结构，写入pdi这个目标inode结构，注意pdi是持有锁的
int add_dirent_entry(struct inode *pdi, char *name, uint inum)
{
	struct dirent dir;
	struct inode *pi;

	if ((pi = dir_find(pdi, name, NULL)) != NULL)
	{
		if (inode_reduce_ref(pi) == -1) // 没加锁，所以只减引用计数
			return -1;
		return -1;
	}
	// pi == NULL 代表该目录里面没有这个名字的目录项

	/* 注意size对于目录是分配数据块的大小，是BLOCK_SIZE的倍数，而对于文件才是真实的数据大小。

	 * 这里有两种情况，一种是索引上已经分配了数据块，那么readinode会成功读，然后比较inum和name如果为空
	 * 代表该目录项空可以用；另一种索引上还没有分配数据块，那么之后writeinode会一次分一个数据块。
	 */
	int off = 0;
	for (; off < pdi->dinode.size; off += sizeof(struct dirent))
	{
		if (readinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
			return -1;
		if (dir.inum == 0 && *dir.name == '\0') // 判断name是为了防止根目录，所以unlink一定要清空inum和name
			break;
	}

	strncpy(dir.name, name, MAX_NAME);
	dir.inum = inum;
	if (writeinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
		return -1;

	return 0;
}

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *inner_create(const char *path, short type)
{
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return NULL;

	if (inode_lock(dir_pinode) == -1) // 对dp加锁
		return NULL;

	// 已存在该 name 的文件，直接返回这个（类似 O_CREATE 不含 O_EXEC 的语义）
	if ((pinode = dir_find(dir_pinode, basename, NULL)) != NULL) // dir_find获取的inode是没有加锁过的
	{
		if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
			goto bad;

		if (inode_lock(pinode) == -1) // 为了保证如果成功返回是有锁的，保持一致
			goto bad;

		if (pinode->dinode.type == type)
			return pinode;

		if (inode_unlock_then_reduce_ref(pinode) == -1)
			goto bad;
		return NULL;
	}

	// 这个name的文件不存在，所以新建一个 inode 表示这个文件，然后在文件所在目录里面新建一个目录项，指向这个 inode

	// 分配一个新的inode(一定是icache不命中)，内部通过iget得到缓存中的inode结构(此时valid为0，
	// 实际内容未加载到内存中)，然后未持有锁，且引用计数为1
	if ((pinode = inode_allocate(type)) == NULL) // 返回的是没有加锁的inode指针
		goto bad;

	if (inode_lock(pinode) == -1) // 加锁
		goto bad;

	pinode->dinode.nlink = 1;

	if (type == FILE_DIR) // 添加 . 和 .. 目录项
	{
		++pinode->dinode.nlink;		// . 指向自己
		++dir_pinode->dinode.nlink; // .. 指向上一级目录
		if (inode_update(dir_pinode) == -1)
			goto bad;

		if (add_dirent_entry(pinode, ".", pinode->inum) == -1 || add_dirent_entry(pinode, "..", dir_pinode->inum) == -1)
			goto bad;
	}
	if (inode_update(pinode) == -1)
		goto bad;

	if (add_dirent_entry(dir_pinode, basename, pinode->inum) == -1) // wrong??
		goto bad;

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
		return NULL;
	return pinode;

bad:
	inode_unlock_then_reduce_ref(dir_pinode);
	return NULL;
}

/* unlink删除目录项，并降低引用，userspace_fs_unlink和userspace_fs_rmdir都是通过内部函数实现 */
int inner_unlink(const char *path)
{
	/* 我们这里只需要删除上一级目录的数据块中的该目录项，删除目录项记得要把name也清空，
	 * 因为inum为0并不能完全代表未使用的情况，这里有点设计失误，然后减少该文件的inode
	 * 的硬链接引用计数，然后成功返回，这里并不发生实际删除。
	 *
	 * 之后的 inode_reduce_ref() （包括该函数里面的这次调用）降低inode的引用
	 * 如果发现ref和nlink都为0，这就会具体地释放inode所占的数据块，位图设置为0。
	 */
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];
	struct dirent dirent;
	uint offset;

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return -1;
	if (inode_lock(dir_pinode) == -1) // 对dp加锁
		return -1;

	if ((pinode = dir_find(dir_pinode, basename, &offset)) == NULL) // wrong??
		goto bad;

	// rmdir无法删除.和..，返回-EINVAL告知libfuse错误
	if (!strncmp(".", basename, MAX_NAME) || !strncmp("..", basename, MAX_NAME))
	{
		inode_unlock_then_reduce_ref(dir_pinode);
		return -EINVAL;
	}

	if (inode_lock(pinode) == -1)
		goto bad;

	// 如果目录项不为空那么无法unlink该目录
	if (pinode->dinode.type == FILE_DIR && !dir_is_empty(pinode))
	{
		inode_unlock_then_reduce_ref(pinode);
		goto bad;
	}

	// 清除所在上一级目录的该目录项
	memset(&dirent, 0, sizeof(struct dirent));
	if (writeinode(dir_pinode, &dirent, offset, sizeof(struct dirent)) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		goto bad;
	}
	if (pinode->dinode.type == FILE_DIR)
	{
		--dir_pinode->dinode.nlink;
		if (inode_update(dir_pinode) == -1) // 对于 ".." 减一次上级目录inode的硬链接计数
		{
			inode_unlock_then_reduce_ref(pinode);
			goto bad;
		}
		--pinode->dinode.nlink; // 对于 "." 减一次该目录inode的硬链接计数
		if (inode_update(pinode) == -1)
		{
			inode_unlock_then_reduce_ref(pinode);
			goto bad;
		}
	}

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	if (pinode->dinode.nlink < 1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}
	// 减硬链接数s
	--pinode->dinode.nlink;
	if (inode_update(pinode) == -1)
	{
		inode_unlock_then_reduce_ref(pinode);
		return -1;
	}

	if (inode_unlock_then_reduce_ref(pinode) == -1)
		return -1;
	return 0;

bad:
	inode_unlock_then_reduce_ref(dir_pinode);
	return -1;
}

/* 判断目录项是否为空（除了"." ".."），调用时已加锁 */
static int dir_is_empty(struct inode *pdi)
{
	struct dirent dirent;
	for (uint offset = 0; offset < pdi->dinode.size; offset += sizeof(struct dirent))
	{
		if (readinode(pdi, &dirent, offset, sizeof(struct dirent)) == -1)
			return -1;
		if (!strncmp(".", dirent.name, MAX_NAME) || !strncmp("..", dirent.name, MAX_NAME))
			continue;
		if (dirent.inum != 0 && *dirent.name != '\0') // 通过inum和name判断是否空闲，设计失误（inum为0不一定是空闲），必须要判断name
			return 0;
	}
	return 1;
}
