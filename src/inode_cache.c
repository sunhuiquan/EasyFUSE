#include "inode_cache.h"

struct inode_cache icache;

/**
 * 返回缓存中的对应inum的inode结构，如果不命中，那么会返回对应inum的inode结构，
 * 但是此时是们没有把数据从disk_inode加载进来，valid是0
 */
static struct inode *
iget(uint dev, uint inum)
{
	struct inode *ip, *empty;

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
		if (empty == 0 && ip->ref == 0) // 记录空的缓存，如果cache不命中使用
			empty = ip;
	}

	// 缓存不命中
	if (empty == 0)
		return (void *)0;

	ip = empty;
	ip->dev = dev;
	ip->inum = inum;
	ip->ref = 1;
	ip->valid = 0; // 磁盘数据未加载到cache
	// to do icache解锁

	return ip;
}