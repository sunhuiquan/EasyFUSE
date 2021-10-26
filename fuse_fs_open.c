#include "fuse_fs_open.h"
#include "inode.h"
#include "inode_cache.h"
#include "fs.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

int create(char *path, ushort type)
{
	struct inode *dir_pinode; // 所在上级目录的inode
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return -1;

	// to do
}

struct inode *find_dir_inode(char *path, char *name)
{
	struct inode *pinode, *next;
	if (pinode == NULL)
		return NULL;

	pinode = iget(ROOT_INODE);
	while ((path = current_dir_name(path, name)) != NULL)
	{
		// name 是当前层的中间name
		// 路径中间的一个name不是目录文件，错误
		if (pinode->dinode.type != FILE_DIR)
		{
			// to do iput 降低iget获取的缓存块的引用计数
			return NULL;
		}

		// 在name为最后一个文件名的时候进入，在下一个 dir_find 前 return 了，返回的是所在目录的inode，如果不中途返回那么 dir_find 这个 name 就是返回该文件的 inode
		if (*path == '\0')
		{
			return pinode;
		}

		// 查找目录项，通过已有的该目录下的 inode 获取对应 name 的 inode
		if ((next = dir_find(pinode, name)) == NULL)
		{
			// 找不到该 name 对应的目录项，错误
			// to do iput 降低iget获取的缓存块的引用计数
			return NULL;
		}
		pinode = next;
	}
	return pinode;
}

struct inode *iget(uint inum)
{
	struct inode *pinode, *pempty = NULL;

	// 对 icache 加锁

	for (pinode = &icache.inodes[0]; pinode < &icache.inodes[CACHE_INODE_NUM]; ++pinode)
	{
		// 缓存命中
		if (pinode->ref > 0 && pinode->inum == inum)
		{
			++pinode->ref;
			// 对 icache 释放锁
			return pinode;
		}
		if (pempty == NULL && pinode->ref == 0)
			pempty = pinode;
	}

	// 缓存不命中
	pempty->valid = 0;	 // 未从磁盘加载入这个内存中的 icache
	pempty->inum = inum; // 现在这个空闲cache块开始缓存inum号inode
	pempty->ref = 1;
	// 对 icache 释放锁
	return pempty;
}

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => NULL
 * 			"/a/"  => path:""  name"a"
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
 * 通过目录 inode 查找该目录下对应 name 的 inode 并返回
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
		// if(readi()){ 	// to do 读inode的内容
		// 	return NULL;
		// }
		if (dirent.inum != 0 && !strncmp(dirent.name, name, MAX_NAME))
			return iget(dirent.inum);
	}
	return NULL;
}
