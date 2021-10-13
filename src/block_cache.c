#include "block_cache.h"

struct cache_list bcache;

/* 初始化缓存块结构 */
void init_block_cache_block()
{
	// 初始化用于 LRU 的双向链表
	bcache.head.prev = &bcache.head;
	bcache.head.next = &bcache.head;
	for (int i = 0; i < CACHE_BLOCK_NUM; ++i)
	{
		// to do
	}
}
