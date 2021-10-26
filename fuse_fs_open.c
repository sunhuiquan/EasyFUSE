#include "fuse_fs_open.h"
#include "inode.h"
#include "inode_cache.h"
#include "fs.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

int create(char *path, ushort type)
{
	struct inode *dir_inode; // 所在上级目录的inode
	char dir_name[MAX_NAME], file_name[MAX_NAME];

	// !!! to do 这里是不可重入的函数，会产生竞争条件，之后别忘了改
	strncpy(dir_name, dirname(path), MAX_NAME);
	strncpy(file_name, basename(path), MAX_NAME);

	if ((dir_inode = find_dir_inode(dir_name)) == NULL)
		return -1;

	// to do
}

struct inode *find_dir_inode(const char *dirname)
{
	struct inode *pinode = iget(ROOT_INODE);
	if (pinode == NULL)
		return NULL;

	// to do
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