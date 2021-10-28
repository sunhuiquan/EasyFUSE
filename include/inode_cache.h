/* 对磁盘中disk_inode记录的缓存，注意一个块是1024B,而一个disk_inode的大小是64B */

#ifndef INODE_CACHE_H
#define INODE_CACHE_H

#include "inode.h"
#include <pthread.h>

extern struct inode_cache icache;

#define CACHE_INODE_NUM 50

struct inode_cache
{
	pthread_mutex_t cache_lock; // pthread互斥锁对整个 inode 缓存结构加锁
	struct inode inodes[CACHE_INODE_NUM]; // 缓存在内存中的真实存放位置，是一个全局变量
};

#endif