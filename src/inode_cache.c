#include "../include/inode_cache.h"

#include <stdlib.h>

struct inode_cache icache;

/* 初始化 inode cache 缓存，其实就是初始化它的互斥锁 */
int inode_cache_init()
{
	// 初始化单个inode结构的锁，注意这个锁是对应于一个内存中的inode结构实体说的，意思是这个
	// 和inode的内容无关，也就是引用计数减到0被重用了，这个锁结构也不会destroy，而是一直存在
	// 只有当整个程序结束才释放这个互斥锁
	int e;
	for (int i = 0; i < CACHE_INODE_NUM; ++i)
		if ((e = pthread_mutex_init(&icache.inodes[i].inode_lock, NULL)) != 0)
			return e;

	// 初始化整个inode cache的锁
	return pthread_mutex_init(&icache.cache_lock, NULL);
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

// void iupdate(struct inode *ip)
// {
// 	struct buf *bp;
// 	struct dinode *dip;

// 	bp = bread(ip->dev, IBLOCK(ip->inum, sb));
// 	dip = (struct dinode *)bp->data + ip->inum % IPB;
// 	dip->type = ip->type;
// 	dip->major = ip->major;
// 	dip->minor = ip->minor;
// 	dip->nlink = ip->nlink;
// 	dip->size = ip->size;
// 	memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
// 	log_write(bp);
// 	brelse(bp);
// }

// void itrunc(struct inode *ip)
// {
// 	int i, j;
// 	struct buf *bp, *bp2;
// 	uint *a, *a2;

// 	for (i = 0; i < NDIRECT; i++)
// 	{
// 		if (ip->addrs[i])
// 		{
// 			bfree(ip->dev, ip->addrs[i]);
// 			ip->addrs[i] = 0;
// 		}
// 	}

// 	if (ip->addrs[NDIRECT])
// 	{
// 		bp = bread(ip->dev, ip->addrs[NDIRECT]);
// 		a = (uint *)bp->data;
// 		for (j = 0; j < NINDIRECT; j++)
// 		{
// 			if (a[j])
// 				bfree(ip->dev, a[j]);
// 		}
// 		brelse(bp);
// 		bfree(ip->dev, ip->addrs[NDIRECT]);
// 		ip->addrs[NDIRECT] = 0;
// 	}

// 	if (ip->addrs[NDIRECT + 1])
// 	{
// 		bp = bread(ip->dev, ip->addrs[NDIRECT + 1]);
// 		a = (uint *)bp->data;
// 		for (j = 0; j < NINDIRECT; ++j)
// 		{
// 			if (a[j])
// 			{
// 				bp2 = bread(ip->dev, a[j]);
// 				a2 = (uint *)bp2->data;

// 				for (int k = 0; k < NINDIRECT; ++k)
// 				{
// 					if (a2[k])
// 						bfree(ip->dev, a2[k]);
// 				}

// 				brelse(bp2);
// 				bfree(ip->dev, a[j]);
// 			}
// 		}
// 		brelse(bp);
// 		bfree(ip->dev, ip->addrs[NDIRECT + 1]);
// 		ip->addrs[NDIRECT + 1] = 0;
// 	}

// 	ip->size = 0;
// 	iupdate(ip);
// }

// void iput(struct inode *ip)
// {
// 	// acquire(&icache.lock);

// 	if (ip->ref == 1 && ip->valid && ip->dinode.nlink == 0)
// 	{
// 		// inode has no links and no other references: truncate and free.

// 		// ip->ref == 1 means no other process can have ip locked,
// 		// so this acquiresleep() won't block (or deadlock).
// 		// acquiresleep(&ip->lock);

// 		// release(&icache.lock);

// 		itrunc(ip);
// 		ip->type = 0;
// 		iupdate(ip);
// 		ip->valid = 0;

// 		// releasesleep(&ip->lock);

// 		// acquire(&icache.lock);
// 	}

// 	ip->ref--;
// 	// release(&icache.lock);
// }
