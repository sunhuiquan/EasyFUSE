#include "../include/inode_cache.h"

#include <stdlib.h>

struct inode_cache icache;

/* 初始化 inode cache 缓存，其实就是初始化它的互斥锁 */
int inode_cache_init()
{
	// 初始化单个inode结构的锁，注意这个锁是对应于一个内存中的inode结构实体说的，意思是这个
	// 和inode的内容无关，也就是引用计数减到0被重用了，这个锁结构也不会destroy，而是一直存在
	// 只有当整个程序结束才释放这个互斥锁
	for (int i = 0; i < CACHE_INODE_NUM; ++i)
		if (pthread_mutex_init(&icache.inodes[i].inode_lock, NULL) != 0)
			return -1;

	// 初始化整个inode cache的锁
	if (pthread_mutex_init(&icache.cache_lock, NULL) != 0)
		return -1;
	return 0;
}

/**
 * 返回缓存中的对应inum的inode结构，如果不命中，那么会返回对应inum的inode结构，
 * 但是此时是们没有把数据从disk_inode加载进来，valid是0
 */
static struct inode *
iget(uint dev, uint inum)
{
	struct inode *ip, *empty = NULL;

	// to do icache加锁

	// 如果已cache，那么返回在缓存中的inode指针
	empty = 0;
	for (int i = 0; i < CACHE_INODE_NUM; ++i)
	{
		ip = &icache.inodes[i];
		if (ip->ref > 0 && ip->dev == dev && ip->inum == inum)
		{
			++ip->ref; // inode cache引用计数增加，这里是cache的引用计数，和硬链接无关，只是为了方便cache的元素的回收利用
			// to do icache解锁
			return ip;
		}
		if (empty == NULL && ip->ref == 0) // 记录空的缓存，如果cache不命中使用
			empty = ip;
	}

	// 缓存不命中
	if (empty == NULL)
		return NULL;

	ip = empty;
	ip->dev = dev;
	ip->inum = inum;
	ip->ref = 1;
	ip->valid = 0; // 磁盘数据未加载到cache

	// to do icache解锁

	return ip;
}
