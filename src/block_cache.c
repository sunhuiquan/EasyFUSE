#include "block_cache.h"
// #include "disk.h"

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

/* 如果已经缓存，那么直接从缓存里面拿出来；如果没缓存，那么取得一个空闲的（之后就可以再把磁盘内容读取后写到这上面） */
struct cache_block *cache_block_get(int blockno)
{
	
}
