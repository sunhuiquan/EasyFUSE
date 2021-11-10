/* Inode缓存层(block layer)代码，提供磁盘上的Inode结构加载到内存的缓存机制，不过实际上Inode读写
 * 请求的操作是通过 block layer 这一中间层实现的，会先读到 block cache，然后再从 block cache
 * 读到 inode cache。
 */

#include "inode_cache.h"
#include "disk.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <util.h>

#define INODE_NUM_PER_BLOCK (BLOCK_SIZE / sizeof(struct disk_inode))				  // 一个块里面的disk_inode结构数
#define INODE_NUM(inum, sb) (((inum) / INODE_NUM_PER_BLOCK) + sb.inode_block_startno) // 得到inum对应的逻辑磁盘块号

_FILE_BLOCK_NUM (NDIRECT + NINDIRECT * (BLOCK_SIZE / sizeof(uint))) // 一个文件最多拥有的块数
#define FILE_SIZE_MAX (MAX_FILE_BLOCK_NUM * BLOCK_SIZE)						   // 一个文件的最大大小

// static 函数，仅在此源文件范围内使用，不放在头文件
static int get_data_blockno_by_inode(struct inode *pi, uint off);

static struct inode_cache icache;

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

/* 取出对应inum的inode内存结构，命中则直接取出，没命中则分配好缓存后去除(不过磁盘inode内容并未加载)，
 * 返回的inode指针是未加锁的。
 *
 * 另外第一次(缓存没命中的那次)valid为0，但不用担心，当我们使用这个inode指针的时候，就需要加锁，
 * 加锁内部会判断valid，为0则加载磁盘内容到缓存里面
 */
struct inode *iget(uint inum)
{
	struct inode *pinode, *pempty = NULL;

	if (pthread_mutex_lock(&icache.cache_lock) != 0)
		return NULL;

	for (pinode = &icache.inodes[0]; pinode < &icache.inodes[CACHE_INODE_NUM]; ++pinode)
	{
		// 缓存命中
		if (pinode->ref > 0 && pinode->inum == inum)
		{
			++pinode->ref;
			if (pthread_mutex_unlock(&icache.cache_lock) != 0)
				return NULL;
			return pinode;
		}
		if (pempty == NULL && pinode->ref == 0)
			pempty = pinode;
	}

	/* 没有对valid和inum更改给inode加锁，是因为icache锁保证了一个缓存不命中的，在第一次iget返回前，
	 * 只有一个进程能够可见这个新的inode，所以无需加锁。
	 *
	 * 没有对ref加锁，是因为我们这整个FS都做到修改一个ref前对整个icache加锁，而icache的锁的范围覆盖了
	 * 单个inode的锁的范围，自然肯定是原子性的。
	 */

	// 缓存不命中
	pempty->valid = 0;	 // 未从磁盘加载入这个内存中的 icache
	pempty->inum = inum; // 现在这个空闲cache块开始缓存inum号inode
	pempty->ref = 1;

	if (pthread_mutex_unlock(&icache.cache_lock) != 0)
		return NULL;

	return pempty;
}

// 降低引用计数，并如果硬链接数和引用计数都为0时，释放对应磁盘
int inode_reduce_ref(struct inode *pi)
{
	// 我们可能要回收某一个缓存块，这个加锁是为了保证避免竞争使多次回收错误
	if (pthread_mutex_lock(&icache.cache_lock) != 0)
		return -1;

	// 注意，如果引用计数为0说明可以回收复用缓存了，不过我们不需要在这里做任何事情，因为关于缓存的回收
	// 是写在 iget() 里面的，那里发现 ref 为0会自动复用
	--pi->ref;

	/* 如果硬链接数和引用计数都为0时，释放对应磁盘结构，不要要知道这里只释放磁盘 inode 结构，
	 * 关于缓存释放是在 iget() 里面的。
	 *
	 * 然后是pi->valid必须为1，代表执行了inode_load，因为硬链接计数是那之后才加载进来的，如果
	 * pi->valid为0，单纯把引用计数降低为0让之后被释放复用即可，不过这里并不会影响我们的程序，
	 * 因为无论想要做什么，即使是单纯删除硬链接，都是要先inode_lock,那里面会调用inode_load。
	 *
	 * 第二个需要注意的点是，即使unlink一个文件，让文件的硬链接数为0，但如果缓存的引用计数不为时，
	 * 仍然不是一个释放磁盘inode块的时机，这是为了保证临时文件的正常使用。
	 * linux上有临时文件的常用用法，就是open O_CREATE得到fd后立即unlink删除目录项(也就是硬链接)，
	 * 即使那是唯一的硬链接，此时内核是不会释放磁盘空间的，因为还有fd引用，通过这个fd可以正常使用
	 * 这个临时文件，只有所有引用(fd关闭)为0，而且nlink也为0，这个时候才是真正释放磁盘空间的时机。
	 */
	if (pi->ref == 0 && pi->valid && pi->dinode.nlink == 0)
	{
		// 这是因为中间不涉及对icache的操作，而且接下来的几个操作耗时间也不小，这里是为了细粒度的临界区，提高并发性
		if (pthread_mutex_unlock(&icache.cache_lock) != 0)
			return -1;
		if (pthread_mutex_lock(&pi->inode_lock) != 0)
			return -1;

		inode_free_address(pi); // 释放所有inode持有的数据块，同时把addr数组清零
		pi->dinode.type = 0;	// 让磁盘的该dionde结构可以被下次的inode_allocate分配重用
		inode_update(pi);		// 把dinode结构写到对应磁盘（实际是bcache）

		if (pthread_mutex_unlock(&pi->inode_lock) != 0)
			return -1;
		if (pthread_mutex_lock(&icache.cache_lock) != 0)
			return -1;
	}
	if (pthread_mutex_unlock(&icache.cache_lock) != 0)
		return -1;
	return 0;
}

/* 释放inode所指向的所有数据块，注意调用这个的时候一定要持有pi的锁。 */
int inode_free_address(struct inode *pi)
{
	struct cache_block *bbuf;
	uint *pui;

	// 一定要持有pi的锁
	for (int i = 0; i < NDIRECT; ++i)
		if (pi->dinode.addrs[i])
		{
			if (block_free(pi->dinode.addrs[i]) == -1) // 释放数据块
				return -1;
			pi->dinode.addrs[i] = 0;
		}

	if (pi->dinode.addrs[NDIRECT])
	{
		if ((bbuf = block_read(pi->dinode.addrs[NDIRECT])) == NULL)
			return -1;
		pui = (uint *)bbuf->data;
		for (; pui < (uint *)&bbuf->data[BLOCK_SIZE]; ++pui) // pui指向存值的地址，之后解引用即可得到值
			if (*pui)
			{
				if (block_free(*pui) == -1) // 释放数据块
				{
					block_unlock_then_reduce_ref(bbuf);
					return -1;
				}
			}
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
		if (block_free(pi->dinode.addrs[NDIRECT]) == -1) // 释放这个二级索引数据块
			return -1;
		pi->dinode.addrs[NDIRECT] = 0;
	}
	pi->dinode.size = 0;
	if (inode_update(pi) == -1)
		return -1;
	return 0;
}

/* 把dinode结构写到对应磁盘（虽然实际上是写入block_cache缓存，还没有实际写入磁盘中，
 * 但从inode layer级别上来看是认为写入磁盘了），注意这个是inode本而之前的wrietinode
 * 写的是指向的数据块中的数据，注意调用这个的时候一定要持有pi的锁。
 *
 * 只要修改了 struct inode 中的 dinode 字段，就会调用 inode_update，因此在inode层上
 * 来看是直写的（一旦改变就立即写入），虽然简单（因为立即保证了写入磁盘），但看上来直写
 * 是低效率的，但实际上并不用担心这个效率，因为实际上是写入bcache缓存，没有实际写入磁盘，
 * 之后是通过block cache层再实际写到磁盘中的。
 *
 * 而block cache层的写入磁盘是借助日志层，而不是直接写入。
 */
int inode_update(struct inode *pi)
{
	// 一定要持有pi的锁
	struct cache_block *bbuf;
	if ((bbuf = block_read(pi->inum + superblock.inode_block_startno)) == NULL)
		return -1;
	memmove(&bbuf->data[(pi->inum % INODE_NUM_PER_BLOCK) * sizeof(struct disk_inode)],
			&pi->dinode, sizeof(struct disk_inode)); // 放入bcache缓存中，后面会实际写入磁盘
	if (write_log_head(bbuf) == -1)
	{
		block_unlock_then_reduce_ref(bbuf);
		return -1;
	}
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
	return 0;
}

/* 释放inode的锁并减引用 */
int inode_unlock_then_reduce_ref(struct inode *pi)
{
	if (inode_unlock(pi) == -1)
		return -1;
	// unlock要在reduce_ref之前，避免重复加锁
	if (inode_reduce_ref(pi) == -1)
		return -1;
}

/* 在磁盘中找到一个未被使用的disk_inode结构，然后加载入内容并返回,通过iget返回，未持有锁且引用计数加一 */
struct inode *inode_allocate(ushort type)
{
	uint i, j;
	struct cache_block *bbuf;
	struct disk_inode *pdi;

	for (i = 0; i < superblock.inode_block_num; ++i)
	{
		if ((bbuf = block_read(superblock.inode_block_startno + i)) == NULL)
			return NULL;

		// 对该块上的 INODE_NUM_PER_BLOCK(16) 个 disk inode 结构遍历
		for (int j = 0; j < INODE_NUM_PER_BLOCK; ++j)
		{
			pdi = (struct disk_inode *)&bbuf->data[j * sizeof(struct disk_inode)];
			if (pdi->type == 0)
			{
				memset(pdi, 0, sizeof(struct disk_inode));
				pdi->type = type;
				if (write_log_head(bbuf) == -1)
				{
					block_unlock_then_reduce_ref(bbuf);
					return NULL;
				}
				if (block_unlock_then_reduce_ref(bbuf) == -1)
					return NULL;
				return iget((i - superblock.inode_block_startno) * INODE_NUM_PER_BLOCK + j);
			}
		}

		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return NULL;
	}
	return NULL; // 磁盘上无空闲的disk inode结构了
}

/* 如果未加载到内存，那么将磁盘上的 dinode 加载到内存 icache 缓存中，这个加载的功能集成到ilock里面了，
 * 因为如果要使用，肯定是要先上锁，上锁的同时顺便就检测加载了
 */
static int inode_load(struct inode *pi)
{
	if (pi->valid == 0)
	{
		struct cache_block *bbuf;
		struct disk_inode *dibuf;
		if ((bbuf = block_read(INODE_NUM(pi->inum, superblock))) == NULL) // 读取对应的inode记录所在的逻辑块，这里block_read以及加载入内存
			return -1;

		dibuf = (struct disk_inode *)(&bbuf->data[(pi->inum % INODE_NUM_PER_BLOCK) * sizeof(struct disk_inode)]);
		memcpy(&pi->dinode, dibuf, sizeof(struct disk_inode)); // 把数据块缓存上的内容拷贝到inode缓存的dinode字段里面

		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;

		pi->valid = 1;
		if (pi->dinode.type == 0) // 磁盘上对应的inode记录为未被使用
			return -1;
	}
	return 0;
}

/* 对内存中的inode对象加锁，然后如果未加载到内存，那么将磁盘上的内容加载到内存中，
 * 把 valid 等于0的情况判断是否加载到内存集成到加锁里面，是因为如果要操作inode的情况，
 * 都要加锁保证原子性，这样顺便检测是否已加载入就可以了
 */
int inode_lock(struct inode *pi)
{
	if (pi == NULL || pi->ref < 1)
		return -1;

	if (pthread_mutex_lock(&pi->inode_lock) != 0) // 加锁
		return -1;

	// 在加锁后，避免竞争导致多次 inode_load 的错误
	return inode_load(pi);
}

/* 对内存中的inode对象解锁*/
int inode_unlock(struct inode *pi)
{
	if (pi == NULL || pi->ref < 1)
		return -1;
	if (pthread_mutex_unlock(&pi->inode_lock) != 0)
		return -1;
	return 0;
}

// 读 inode 里面的数据，实际上是通过 inode 和对应偏移量得到对应数据块的位置，然后读数据块
int readinode(struct inode *pi, void *dst, uint off, uint n)
{
	uint blockno;
	struct cache_block *bbuf;
	int readn, len;

	if (n < 0 || off > pi->dinode.size)
		return -1;
	if (off + n > pi->dinode.size)
		n = pi->dinode.size - off;

	for (readn = 0; readn < n; readn += len, off += len, dst += len)
	{
		// 得到所在偏移量所在的块，并读到缓存，得到的缓存块是持有着锁的
		if ((blockno = get_data_blockno_by_inode(pi, off)) == -1)
			return -1;

		if ((bbuf = block_read(blockno)) == NULL)
			return -1;
		// 拷贝具体的该块内的偏移量的数据
		len = min(n - readn, BLOCK_SIZE - off % BLOCK_SIZE);
		memmove((char *)dst, bbuf->data + (off % BLOCK_SIZE), len);

		if (block_unlock_then_reduce_ref(bbuf) == -1) // 解锁并减少引用计数（因为不再使用）
			return -1;
	}
	return readn;
}

/* 向 inode 对应数据块写数据，实际上是通过 inode 和对应偏移量得到对应数据块的位置，
 * 然后写这个数据块。
 *
 * 看起来是直接写到对应磁盘（虽然实际上是写入block_cache缓存，还没有实际写入磁盘中，
 * 但从inode layer级别上来看是认为写入磁盘了）
 */
int writeinode(struct inode *pi, void *src, uint off, uint n)
{
	uint blockno;
	struct cache_block *bbuf;
	int writen, len;

	if (n < 0 || off > pi->dinode.size)
		return -1;
	if (off + n > MAX_FILE_BLOCK_NUM * BLOCK_SIZE)
		return -1;

	for (writen = 0; writen < n; writen += len, off += len, src += len)
	{
		// 得到所在偏移量所在的块，并读到缓存
		if ((blockno = get_data_blockno_by_inode(pi, off)) == -1)
			return -1;
		if ((bbuf = block_read(blockno)) == NULL)
			return -1;
		len = min(n - writen, BLOCK_SIZE - off % BLOCK_SIZE);
		memmove(bbuf->data + (off % BLOCK_SIZE), src, len);
		if (write_log_head(bbuf) == -1)
		{
			block_unlock_then_reduce_ref(bbuf);
			return -1;
		}
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
	}
	if (off > pi->dinode.size)
		pi->dinode.size = off;

	if (inode_update(pi) == -1)
		return -1;
	return writen;
}

/* 根据inode的dinode的address的索引，和该文件的偏移量得到对应的数据块号 */
static int get_data_blockno_by_inode(struct inode *pi, uint off)
{
	uint blockno; // 数据块号
	struct cache_block *bbuf;
	uint *indirect_addrs;

	if (off < 0 || off > FILE_SIZE_MAX)
		return -1;

	int boff = off / BLOCK_SIZE; // 得到是第几个数据块（数据块偏移量）
	if (boff < NDIRECT)			 // 直接指向
	{
		if ((blockno = pi->dinode.addrs[boff]) == 0)
			// 对应的地址未指向数据块，则分配一个并返回
			blockno = pi->dinode.addrs[boff] = balloc();
		return blockno;
	}
	boff -= NDIRECT;

	// 我们用 uint 作为块号的单位，所以是 BLOCK_SIZE / sizeof(uint)
	if (boff < (NINDIRECT * (BLOCK_SIZE / sizeof(uint)))) // 二级引用
	{
		if ((blockno = pi->dinode.addrs[NDIRECT]) == 0)
			blockno = pi->dinode.addrs[NDIRECT] = balloc();
		bbuf = block_read(blockno); // 读数据块
		if (bbuf == NULL)
			return -1;

		indirect_addrs = (uint *)bbuf->data;
		// 这里转成 uint 数组形式的作用是，这样通过下标得到的地址就是 index * sizeof(struct uint) 了，要不然你要手动 *4 因为原本是 char 数组的形式
		if ((blockno = indirect_addrs[boff]) == 0)
		{
			// 对应的地址未指向数据块，则分配一个并返回
			blockno = indirect_addrs[boff] = balloc();
			if (write_log_head(bbuf) == -1)
			{
				block_unlock_then_reduce_ref(bbuf);
				return -1;
			}
		}
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
		return blockno;
	}
	return -1;
}