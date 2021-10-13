#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include "fs.h"

#define CACHE_BLOCK_NUM 100 // 缓冲的块数，这里是一个写死的值
// 这个值并不高效，实际中应该分析机器的配置、磁盘的大小等因素而得

struct cache_block // 逻辑块在内存中的结构
{
	uint refcnt;	 // 引用计数，到 0 的时候代表释放可重用
	uint is_changed; // 脏页标志
	struct cache_block *prev;
	struct cache_block *next;
	char data[BLOCK_SIZE]; // 实际 block 的内容，即实际读入/写回磁盘的内容，前几个字段都是内存中才有的
};

struct cache_list
{
	struct cache_block bcaches[CACHE_BLOCK_NUM]; // cache的实体
	struct cache_block head;					 // 双向链表的空头结点,要用双向链表是因为要实现 LRU
};

/* 初始化缓存块结构 */
void init_block_cache_block();

#endif