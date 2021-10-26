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

struct inode *find_dir_inode(const char *path, char *basename)
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

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => NULL
 * 			"/a/"  => path:"a" name""
 * 			"/a/b" => path:"a" name:"b"
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