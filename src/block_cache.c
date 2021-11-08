/* 数据块缓存层(block layer)代码，提供磁盘上的数据块加载到内存的缓存机制。 */

#include "block_cache.h"
#include "disk.h"
#include "log.h"
#include <stdlib.h>

struct cache_list bcache;

/* 初始化缓存块结构 */
int init_block_cache_block()
{
	// 初始化用于 LRU 的双向链表
	bcache.head.prev = &bcache.head;
	bcache.head.next = &bcache.head;
	for (int i = 0; i < CACHE_BLOCK_NUM; ++i)
	{
		bcache.bcaches[i].next = bcache.head.next;
		bcache.bcaches[i].prev = &bcache.head;
		bcache.head.next->prev = &bcache.bcaches[i];
		bcache.head.next = &bcache.bcaches[i];

		if (pthread_mutex_init(&bcache.bcaches[i].block_lock, NULL) != 0) // 初始化单个数据块结构的锁
			return -1;
	}

	// 初始化整个数据块缓存的锁
	if (pthread_mutex_init(&bcache.cache_lock, NULL) != 0)
		return -1;
	return 0;
}

/* 如果已经缓存，那么直接从缓存里面拿出来内存块；如果没缓存，那么取得一个空闲的内存块（之后就可以把磁盘加载到这里）。
 * 注意返回的数据块缓存 cache_block * 是持有锁的状态，同时引用计数也加一（包括从0变成1）。
 */
static struct cache_block *cache_block_get(int blockno)
{
	struct cache_block *pc; // pointer to cache

	/* 为了支持并发要有对这一整个 cache layer 进行加锁 */
	if (pthread_mutex_lock(&bcache.cache_lock) == -1)
		return NULL;

	/* 注意这里不需要 is_cache 为1，因为 blockno 相同的情况代表要么已经加载进内存 is_cache为1，要么是马上要加载，
	 * 另一个进程执行了缓冲不命中的结果，之后 is_cache 暂时为0代表要加载还没加载完，这里需要锁的同步机制，这里一定
	 * 不能认为是缓冲不命中的情况，因为内存中的缓冲里面只能一个时间的一个blockno必须是唯一的，不然会出现数据不一致的情况。
	 *
	 * 至于返回一个 blockno 对应但是还未从磁盘加载进内存的这种错误块的担忧完全不必，因为外面这里仅仅是为了避开可能导致
	 * cache layer 有多个相同 blockno 块的数据不一致问题，进入之后，我们会请求这个块本身的锁，而我们的代码中约定确保了
	 * 如果缓冲没命中，取得一个空闲 cache block 后也会请求加锁，只有而释放锁的时间一定是在把磁盘块加载入内存之后。
	 *
	 * 简而言之，其实释放锁的时机更晚：
	 * 缓冲命中   -> 请求加锁 -> 对这个已经加载进内存的块的操作(上层调用) -> 操作结束后释放锁
	 * 缓冲不命中 -> 请求加锁 -> 从磁盘物理块加载进内存 -> 对这个已经加载进内存的块的操作(上层调用) -> 操作结束后释放锁
	 */
	for (pc = bcache.head.next; pc != &bcache.head; pc = pc->next)
		if (pc->blockno == blockno)
		{
			++pc->refcnt; // 和inode同样，把引用计数和bcache的锁相关联??
			if (pthread_mutex_unlock(&bcache.cache_lock) == -1)
				return NULL;

			if (pthread_mutex_lock(&pc->block_lock) == -1)
				return NULL;
			return pc; // 缓冲命中，返回的cache_block是持有锁的
		}

	/* is_cache 为 1 代表缓冲着数据，因为之后即使 refcnt 减到0的时候仅仅代表现在没有进程正在引用这个块，
	 * 但是并不释放，而是仍然缓冲着数据，is_cache 仍然是1，之后完全可以再命中。

	 * 通过 LRU 机制找到最久前分配的缓冲块释放，is_cache 变成0（无论之前是否为0），代表此时没有缓冲数据是空闲的，
	 * 之后的其他函数可以通过 is_cache 得知是否缓冲命中，没用命中的话然后会再从磁盘读入再变成1。
	 */

	// 缓存不命中，找到一个空闲缓存块返回
	for (pc = bcache.head.prev; pc != &bcache.head; pc = pc->prev)
		if (pc->refcnt == 0)
		{
			if (pthread_mutex_lock(&bcache.cache_lock) == -1)
				return NULL;

			if (pthread_mutex_lock(&pc->block_lock) == -1)
				return NULL;
			pc->blockno = blockno;
			pc->is_cache = 0;
			pc->refcnt = 1;
			return pc; // 返回的cache_block是持有锁的
		}
	return NULL; // 当前缓存中的空闲块不够使用，返回后要么终止，要么就休眠一会再尝试
}

/* 把磁盘内容读到缓存块上（如果is_cache为0则把磁盘内容读到内存，然后is_cache变成1），并返回这个缓存块，
 *（通过cache_block_get，如果命中则直接返回否则需要找到一个空闲的缓存块返回），之后这个缓冲块如果
 * is_cache为0则要求从磁盘加载内容，另外注意现在进程仍然持有对应数据块缓存的block_lock锁。
 */
struct cache_block *block_read(int blockno)
{
	struct cache_block *pcb = cache_block_get(blockno); // 注意 pcb 这个 cache_block * 持有的锁

	if (!pcb->is_cache) // 未加载磁盘内容到缓存
	{
		if (disk_read(pcb) == -1) // 加载磁盘内容到缓存
			return NULL;
		pcb->is_cache = 1; // 因为cache_block_get返回持有锁，所以安全
	}

	return pcb;
}

/* 释放锁，并减少这个缓存数据块的引用计数，如果引用计数降低到0，那么会回收这个缓存块 */
int block_unlock_then_reduce_ref(struct cache_block *bbuf)
{
	if (pthread_mutex_unlock(&bbuf->block_lock) == -1)
		return -1;

	if (pthread_mutex_lock(&bcache.cache_lock) == -1)
		return -1;
	--bbuf->refcnt;

	if (bbuf->refcnt == 0) // 引用计数降低到0，这个缓存块没有被任何操作使用
	{
		/* 将这个节点放到头结点的下一个的第一个位置，注意虽然refcnt的计数为0，但是里面的
		 * 内容仍是最新的内容，而且没有写回磁盘，下次cache_block_get仍可以缓存命中，
		 * 至于具体的写回磁盘需要手动调用，而不是在此处，这样尽量使用内存中的数据块，而减少
		 * 写回的次数，将多次内存写入组合成一个磁盘写入，从而提高了效率。
		 */
		bbuf->next->prev = bbuf->prev;
		bbuf->prev->next = bbuf->next;
		bbuf->next = bcache.head.next;
		bbuf->prev = &bcache.head;
		bcache.head.next->prev = bbuf;
		bcache.head.next = bbuf;
	}

	if (pthread_mutex_unlock(&bcache.cache_lock) == -1)
		return -1;
	return 0;
}

/* 增加这个缓存数据块引用计数 */
int block_increase_ref(struct cache_block *bbuf)
{
	if (pthread_mutex_lock(&bcache.cache_lock) == -1)
		return -1;
	++bbuf->refcnt;
	if (pthread_mutex_unlock(&bcache.cache_lock) == -1)
		return -1;
	return 0;
}

/* 减少这个缓存数据块的引用计数 */
int block_reduce_ref(struct cache_block *bbuf)
{
	if (pthread_mutex_lock(&bcache.cache_lock) == -1)
		return -1;
	--bbuf->refcnt;
	if (pthread_mutex_unlock(&bcache.cache_lock) == -1)
		return -1;
	return 0;
}
