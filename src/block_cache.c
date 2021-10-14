#include "block_cache.h"
#include "disk.h"
#include <stdlib.h>

struct cache_list bcache;

/* 初始化缓存块结构 */
void init_block_cache_block()
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
	}
}

/* 如果已经缓存，那么直接从缓存里面拿出来内存块；如果没缓存，那么取得一个空闲的内存块（之后就可以把磁盘加载到这里） */
struct cache_block *cache_block_get(int blockno)
{
	struct cache_block *pc; // pointer to cache

	/* 为了支持并发要有对这一整个 cache layer 进行加锁 */
	// to do 给 bcache 加锁

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
	 *
	 * 可见 blockno 对应但是 is_cache 为0还没加载进内存的情况下，进入缓冲命中处理，请求加锁，而别的进程之前缓冲不命中的
	 * 情况已经请求到了锁，所以这个进程要等到缓冲不命中的那个进程释放锁，所以当这个进程之后加锁成功的时候，此时是一定已经从
	 * 磁盘加载进这个 cache block，is_cache变成1了。
	 */
	for (pc = bcache.head.next; pc != &bcache.head; pc = pc->next)
		if (pc->blockno == blockno)
		{
			// to do 释放 cache layer 这个整体的 bcache 锁
			// to do 获取这个 pc 内存块的锁，这样并发的时候一个时间只有一个进程能操作
			++pc->refcnt;
			return pc; // 缓冲命中
		}

	/* is_cache 为 1 代表缓冲着数据，因为之后即使 refcnt 减到0的时候仅仅代表现在没有进程正在引用这个块，
	 * 但是并不释放，而是仍然缓冲着数据，is_cache 仍然是1，之后完全可以再命中。

	 * 通过 LRU 机制找到最久前分配的缓冲块释放，is_cache 变成0（无论之前是否为0），代表此时没有缓冲数据是空闲的，
	 * 之后的其他函数可以通过 is_cache 得知是否缓冲命中，没用命中的话然后会再从磁盘读入再变成1。
	 */
	for (pc = bcache.head.prev; pc != &bcache.head; pc = pc->prev)
		if (pc->refcnt == 0)
		{
			// to do 释放 cache layer 这个整体的 bcache 锁
			// to do 获取这个 pc 内存块的锁
			pc->blockno = blockno;
			pc->is_cache = 0;
			pc->refcnt = 1;
			return pc; // 返回空闲 cache 块
		}

	return NULL; // 当前内存块不够使用，返回后要么终止，要么就休眠一会再尝试
}

/* 把磁盘内容读到缓冲块上（通过cache_block_get，如果命中则直接返回，否则需要从文件读到得到的空闲块）
 * 另外注意现在进程仍然拥有 cacle block 的锁。
 */
struct cache_block *block_read(int blockno)
{
	struct cache_block *pcb = cache_block_get(blockno);
	if (pcb == NULL || !pcb->is_cache)
		return pcb;

	// 缓冲不命中
	if (disk_read(pcb) == -1)
		return NULL;
	pcb->is_cache; // 磁盘内容缓冲进了内存
	return pcb;
}

/* 把内存块的内容写到磁盘上 */
int block_write(struct cache_block *pcb)
{
	// to do 注意这里要确保进程获取了内存块的锁
	// to do 为什么不能解锁而再请求锁，而是需要一直保存锁的原因？
	if (disk_write(pcb) == -1)
		return -1;
	return 0;
}
