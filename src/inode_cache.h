/* 对磁盘中disk_inode记录的缓存，注意一个块是1024B,而一个disk_inode的大小是64B */

#ifndef INODE_CACHE_H
#define INODE_CACHE_H

#include "inode.h"

#define CACHE_INODE_NUM 50

struct inode_cache
{
	// to do 加锁
	struct inode inodes[CACHE_INODE_NUM];
};

#endif