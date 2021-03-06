/* 数据块缓存层(block layer)代码，提供磁盘上的数据块加载到内存的缓存机制。 */

#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include "fs.h"
#include <pthread.h>

#define CACHE_BLOCK_NUM 100 // 缓冲的块数，这里是一个写死的值
// 这个值并不高效，实际中应该分析机器的配置、磁盘的大小等因素而得

struct cache_block // 数据块在内存中的结构
{
	uint blockno;				// 磁盘逻辑块号，用来标识身份
	uint is_cache;				// 是否正在缓冲数据
	uint refcnt;				// 引用计数，到 0 的时候代表释放可重用
	uint is_changed;			// 脏页标志
	pthread_mutex_t block_lock; // 对单个内存中的数据块结构加锁
	struct cache_block *prev;
	struct cache_block *next;
	char data[BLOCK_SIZE]; // 实际 block 的内容，即实际读入/写回磁盘的内容，前几个字段都是内存中才有的
};

struct cache_list
{
	struct cache_block bcaches[CACHE_BLOCK_NUM]; // cache的实体
	struct cache_block head;					 // 双向链表的空头结点,要用双向链表是因为要实现 LRU
	pthread_mutex_t cache_lock;					 // 对整个数据块缓存加锁
};

/* 初始化缓存块结构 */
int init_block_cache_block();

/* 把磁盘内容读到缓冲块上（通过cache_block_get，如果命中则直接返回，否则需要从文件读到得到的空闲块） */
struct cache_block *block_read(int blockno);

/* 释放锁，并减少这个缓存数据块的引用计数，如果引用计数降低到0，那么会回收这个缓存块 */
int block_unlock_then_reduce_ref(struct cache_block *bbuf);

/* 增加这个缓存数据块引用计数 */
int block_increase_ref(struct cache_block *bbuf);

/* 减少这个缓存数据块的引用计数 */
int block_reduce_ref(struct cache_block *bbuf);

#endif
