#include "fuse_fs_open.h"
#include "inode.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "fs.h"
#include "disk.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <pthread.h>

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type)
{
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return -1;

	if (inode_lock(dir_pinode) == -1) // 对dp加锁
		return NULL;

	// 已存在该 name 的文件，直接返回这个（类似 O_CREATE 不含 O_EXEC 的语义）
	if ((pinode = dir_find(dir_pinode, basename)) != NULL) // dir_find获取的inode是没有加锁过的
	{
		if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
			return NULL;

		if (inode_lock(pinode) == -1) // 为了保证如果成功返回是有锁的，保持一致
			return NULL;

		if (pinode->dinode.type == type)
			return pinode;

		if (inode_unlock_then_reduce_ref(pinode) == -1)
			return NULL;
		return NULL;
	}

	// 这个name的文件不存在，所以新建一个 inode 表示这个文件，然后在文件所在目录里面新建一个目录项，指向这个 inode

	// 分配一个新的inode(一定是icache不命中)，内部通过iget得到缓存中的inode结构(此时valid为0，
	// 实际内容未加载到内存中)，然后未持有锁，且引用计数为1
	if ((pinode = inode_allocate(type)) == NULL) // 返回的是没有加锁的inode指针
		return -1;

	if (inode_lock(pinode) == -1) // 加锁
		return NULL;
	pinode->dinode.nlink = 1;
	if (inode_update(pinode) == -1) // 写入磁盘??
		return NULL;

	if (type == FILE_DIR) // 添加 . 和 .. 目录项
	{
		++dir_pinode->dinode.nlink; // .. 指向上一级目录
		// No ip->nlink++ for ".": avoid cyclic ref count.??
		if (inode_update(dir_pinode) == -1) // 写入磁盘
			return NULL;

		if (add_dirent_entry(pinode, ".", pinode->inum) == -1 || add_dirent_entry(pinode, "..", dir_pinode->inum) == -1)
			return -1;
	}

	if (add_dirent_entry(dir_pinode, basename, pinode->inum) == -1)
		return -1;

	if (inode_unlock_then_reduce_ref(dir_pinode) == -1)
		return NULL;

	return pinode;
}

// 返回路径文件所在的目录的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_dir_inode(char *path, char *name)
{
	struct inode *pinode, *next;

	pinode = iget(ROOT_INODE); // iget返回的是未加锁的inode指针
	while ((path = current_dir_name(path, name)) != NULL)
	{
		if (inode_lock(pinode) == -1)
			return NULL;
		// name 是当前层的中间name
		// 路径中间的一个name不是目录文件，错误
		if (pinode->dinode.type != FILE_DIR)
		{
			if (inode_unlock_then_reduce_ref(pinode) == -1)
				return NULL;
			return NULL;
		}

		// 在name为最后一个文件名的时候进入，在下一个 dir_find 前 return 了，返回的是所在目录的inode，如果不中途返回那么 dir_find 这个 name 就是返回该文件的 inode
		if (*path == '\0')
		{
			if (inode_unlock(pinode) == -1) // 解锁
				return NULL;
			// 这里返回的inode*要被使用，可不要减引用，不过要解锁
			return pinode;
		}

		// 查找目录项，通过已有的该目录下的 inode 获取对应 name 的 inode
		if ((next = dir_find(pinode, name)) == NULL)
		{
			if (inode_unlock_then_reduce_ref(pinode) == -1)
				return NULL;
			return NULL;
		}

		if (inode_unlock_then_reduce_ref(pinode) == -1)
			return NULL;
		pinode = next;
	}
	if (inode_reduce_ref(pinode) == -1) // 减引用
		return NULL;
	return NULL;
}

/* 去除对应inum的inode内存结构，命中则直接取出，没命中则分配好缓存后去除(不过磁盘inode内容并未加载)，
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

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => NULL
 * 			"/a/"  => NULL  name"a"
 * 			"/a/b" => path:"b" name:"b"
 * (多余的 '/' 不影响结果，最后如果是 '/' 会忽略，不会当成路径)
 */
char *current_dir_name(char *path, char *name)
{
	char *s;
	int len;

	while (*path == '/')
		path++;
	if (*path == 0) // 空字符串错误
		return NULL;
	s = path;
	while (*path != '/' && *path != 0)
		path++;
	len = path - s;
	if (len >= MAX_NAME)
		memmove(name, s, MAX_NAME);
	else
	{
		memmove(name, s, len);
		name[len] = 0;
	}
	while (*path == '/')
		path++;
	return path;
}

/*!
 * 通过目录 inode 查找该目录下对应 name 的 inode 并返回，通过iget，返回未加锁，且已增加了引用计数。
 * (内存中调用iget就说明有新的对象指向了这个inode缓存，调用iget就会导致引用计数+1)
 *
 * \param pdi A pointer to current directory inode.
 */
struct inode *dir_find(struct inode *pdi, char *name)
{
	struct dirent dirent;

	if (pdi->dinode.type != FILE_DIR)
		return NULL;

	uint sz = pdi->dinode.size;
	for (uint off = 0; off < sz; off += sizeof(struct dirent))
	{
		if (readinode(pdi, &dirent, off, sizeof(struct dirent)) != sizeof(struct dirent))
			return NULL;
		if (dirent.inum != 0 && !strncmp(dirent.name, name, MAX_NAME))
			return iget(dirent.inum);
	}
	return NULL;
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

#define MAX_FILE_BLOCK_NUM (NDIRECT + NINDIRECT * (BLOCK_SIZE / sizeof(uint)))
// 写 inode 里面的数据，实际上是通过 inode 和对应偏移量得到对应数据块的位置，然后写数据块
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
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
	}
	if (off > pi->dinode.size)
		pi->dinode.size = off;

	if (inode_update(pi) == -1) // ??
		return -1;
	return writen;
}

#define FILE_SIZE_MAX ((12 + 256) * BLOCK_SIZE)

int get_data_blockno_by_inode(struct inode *pi, uint off)
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
		}
		// brelse(bbuf);
		return blockno;
	}

	return -1;
}

extern struct super_block superblock;
// int balloc() 通过 bitmap 获取一个空的数据块号，设置 bitmap 对应位为1
int balloc()
{
	struct cache_block *bbuf;
	for (int i = 0; i < superblock.bitmap_block_num; ++i) // bitmap块号，共32个
	{
		if ((bbuf = block_read(superblock.bitmap_block_startno + i)) == NULL)
			return -1;
		for (int j = 0; j < BLOCK_SIZE; ++j)
		{
			char bit = bbuf->data[j];
		}
	}
}

#define INODE_NUM_PER_BLOCK (BLOCK_SIZE / sizeof(struct disk_inode))
#define INODE_NUM(inum, sb) (((inum) / INODE_NUM_PER_BLOCK) + sb.inode_block_startno)

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

int inode_unlock(struct inode *pi)
{
	if (pi == NULL || pi->ref < 1)
		return -1;
	if (pthread_mutex_unlock(&pi->inode_lock) != 0)
		return -1;
	return 0;
}

/* 在磁盘中找到一个未被使用的disk_inode结构，然后加载入内容并返回,通过iget返回，未持有锁且引用计数加一 */
struct inode *inode_allocate(ushort type)
{
	uint i, j;
	struct cache_block *bbuf;
	struct disk_inode *pdi;

	for (i = 0; i < superblock.inode_block_num; ++i)
	{
		if ((bbuf = cache_block_get(superblock.inode_block_startno + i)) == NULL)
			return NULL;

		// 对该块上的 INODE_NUM_PER_BLOCK(16) 个 disk inode 结构遍历
		for (int j = 0; j < INODE_NUM_PER_BLOCK; ++j)
			if (pdi->type == 0)
				return iget((i - superblock.inode_block_startno) * INODE_NUM_PER_BLOCK + j);

		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
	}
	return NULL; // 磁盘上无空闲的disk inode结构了
}

// 将name和inum组合成一条dirent结构，写入pdi这个目标inode结构，注意pdi是持有锁的
int add_dirent_entry(struct inode *pdi, const char *name, uint inum)
{
	struct dirent dir;
	struct inode *pi;

	if ((pi = dir_find(pdi, name)) != NULL)
	{
		if (inode_reduce_ref(pi) == -1) // 没加锁，所以只减引用计数
			return -1;
		return -1;
	}
	// pi == NULL 代表该目录里面没有这个名字的目录项

	int off = 0;
	for (; off < pdi->dinode.size; off += sizeof(struct dirent))
	{
		if (readinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
			return -1;
		if (dir.inum == 0)
			break;
	}

	strncpy(dir.name, name, MAX_NAME);
	dir.inum = inum;
	if (writeinode(pdi, &dir, off, sizeof(struct dirent)) != sizeof(struct dirent))
		return -1;

	return 0;
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
		inode_update(pi);		// 把dinode结构写到对应磁盘

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
		if ((bbuf = cache_block_get(pi->dinode.addrs[NDIRECT])) == NULL)
			return -1;
		pui = (uint *)bbuf->data;
		for (; pui < &bbuf->data[BLOCK_SIZE]; ++pui) // pui指向存值的地址，之后解引用即可得到值
			if (*pui)
			{
				if (block_free(*pui) == -1) // 释放数据块
					return -1;
			}
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
		if (block_free(pi->dinode.addrs[NDIRECT]) == -1) // 释放这个二级索引数据块
			return -1;
		pi->dinode.addrs[NDIRECT] = 0;
	}
	pi->dinode.size = 0; // 之后要iupdate写入磁盘

	return 0;
}

/* 把dinode结构写到对应磁盘，注意这个是inode本身，而之前的wrietinode写的
 * 是指向的数据块中的数据，注意调用这个的时候一定要持有pi的锁。
 */
int inode_update(struct inode *pi)
{
	// 一定要持有pi的锁
	struct cache_block *bbuf;
	if ((bbuf = cache_block_get(pi->inum + superblock.inode_block_startno)) == NULL)
		return -1;
	memmove(&bbuf->data[(pi->inum % INODE_NUM_PER_BLOCK) * sizeof(struct disk_inode)],
			&pi->dinode, sizeof(struct disk_inode)); // 放入bcache缓存中，后面会实际写入磁盘
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
	return 0;
}

int inode_unlock_then_reduce_ref(struct inode *pi)
{
	if (inode_unlock(pi) == -1)
		return -1;
	// unlock要在reduce_ref之前，避免重复加锁
	if (inode_reduce_ref(pi) == -1)
		return -1;
}
