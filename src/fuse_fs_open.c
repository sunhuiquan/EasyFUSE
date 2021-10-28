#include "../include/fuse_fs_open.h"
#include "../include/inode.h"
#include "../include/inode_cache.h"
#include "../include/block_cache.h"
#include "../include/fs.h"
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

struct inode *create(char *path, ushort type)
{
	struct inode *dir_pinode, *pinode;
	char basename[MAX_NAME];

	if ((dir_pinode = find_dir_inode(path, basename)) == NULL)
		return -1;

	// to do 对 dir_pinode 加锁

	// 已存在该 name 的文件，直接返回（open的语义O_CREATE如果没有O_EXCL那么已存在就直接返回）
	if ((pinode = dir_find(dir_pinode, basename)) != NULL)
	{
		// to do dir_pinode 解锁并减引用
		// to do 加锁这里返回的 pinode
		if (pinode->dinode.type == type)
			return pinode;
		// to do 解锁这里返回的 pinode
		return NULL;
	}

	// to do 新建一个 inode 表示这个文件，然后在文件所在目录里面新建一个目录项，指向这个 inode
	if ((pinode = anode_allocate(type)) == NULL)
		return -1;

	// to do 加锁
	pinode->dinode.nlink = 1;
	// to do update

	if (type == FILE_DIR)
	{
	}

	// to do if(dirlink(dp,name,ip->inum)) 添加目录项
	// to do 解锁dp并写回磁盘

	return 0;
}

// 返回路径文件所在的目录的 inode
struct inode *find_dir_inode(char *path, char *name)
{
	struct inode *pinode, *next;
	if (pinode == NULL)
		return NULL;

	pinode = iget(ROOT_INODE);
	while ((path = current_dir_name(path, name)) != NULL)
	{
		// to do 给 pinode 加锁
		// name 是当前层的中间name
		// 路径中间的一个name不是目录文件，错误
		if (pinode->dinode.type != FILE_DIR)
		{
			// to do iput 降低iget获取的缓存块的引用计数
			return NULL;
		}

		// 在name为最后一个文件名的时候进入，在下一个 dir_find 前 return 了，返回的是所在目录的inode，如果不中途返回那么 dir_find 这个 name 就是返回该文件的 inode
		if (*path == '\0')
		{
			return pinode;
		}

		// 查找目录项，通过已有的该目录下的 inode 获取对应 name 的 inode
		if ((next = dir_find(pinode, name)) == NULL)
		{
			// 找不到该 name 对应的目录项，错误
			// to do iput 降低iget获取的缓存块的引用计数
			return NULL;
		}
		pinode = next;
	}
	return pinode;
}

struct inode *iget(uint inum)
{
	struct inode *pinode, *pempty = NULL;

	// 对 icache 加锁

	for (pinode = &icache.inodes[0]; pinode < &icache.inodes[CACHE_INODE_NUM]; ++pinode)
	{
		// 缓存命中
		if (pinode->ref > 0 && pinode->inum == inum)
		{
			++pinode->ref;
			// 对 icache 释放锁
			return pinode;
		}
		if (pempty == NULL && pinode->ref == 0)
			pempty = pinode;
	}

	// 缓存不命中
	pempty->valid = 0;	 // 未从磁盘加载入这个内存中的 icache
	pempty->inum = inum; // 现在这个空闲cache块开始缓存inum号inode
	pempty->ref = 1;
	// 对 icache 释放锁
	return pempty;
}

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名
 *
 * example: "" 	   => NULL
 * 			"/a/"  => path:""  name"a"
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
 * 通过目录 inode 查找该目录下对应 name 的 inode 并返回
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

// 读 inode 里面的数据，实际上是通过 inode 得到对应偏移量的地址的数据块，然后读数据块
int readinode(struct inode *pi, void *dst, uint off, uint n)
{
	uint blockno;
	struct cache_block *bbuf;
	int readn, len;

	if (n <= 0 || off > pi->dinode.size)
		return -1;
	if (off + n > pi->dinode.size)
		n = pi->dinode.size - off;

	for (readn = 0; readn < n; readn += len, off += len, dst += len)
	{
		// 得到所在偏移量所在的块，并读到缓存
		if ((blockno = get_data_blockno_by_inode(pi, off)) == -1)
			return -1;
		if ((bbuf = block_read(blockno)) == NULL)
			return -1;
		// 拷贝具体的该块内的偏移量的数据
		len = min(n - readn, BLOCK_SIZE - off % BLOCK_SIZE);
		memmove((char *)dst, bbuf->data + (off % BLOCK_SIZE), len);
	}
	// 释放 bbuf 锁，在 get_data_blockno_by_inode 内部加的
	return readn;
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

// static uint
// balloc2(uint dev)
// {
// 	int b, bi, m;
// 	struct buf *bp;

// 	bp = 0;
// 	for (b = 0; b < sb.size; b += BPB)
// 	{
// 		bp = bread(dev, BBLOCK(b, sb));
// 		for (bi = 0; bi < BPB && b + bi < sb.size; bi++)
// 		{
// 			m = 1 << (bi % 8);
// 			if ((bp->data[bi / 8] & m) == 0)
// 			{						   // Is block free?
// 				bp->data[bi / 8] |= m; // Mark block in use.
// 				log_write(bp);
// 				brelse(bp);
// 				bzero(dev, b + bi);
// 				return b + bi;
// 			}
// 		}
// 		brelse(bp);
// 	}
// 	panic("balloc: out of blocks");
// }

extern struct super_block superblock;
// int balloc() 通过 bitmap 获取一个空的数据块号
int balloc()
{
	struct cache_block *bbuf;
	for (int i = 0; i < superblock.bitmap_block_num; ++i) // bitmap块号，共32个
	{
		if ((bbuf = block_read(superblock.bitmap_block_startno + i)) == NULL)
			return -1;
		for (int j = 0;;)
		{
		}
	}
}

#define INODE_NUM_PER_BLOCK (BLOCK_SIZE / sizeof(struct disk_inode)
#define INODE_NUM(inum, sb) ((inum) / INODE_NUM_PER_BLOCK) + sb.inode_block_startno)

/* 如果未加载到内存，那么将磁盘上的 dinode 加载到内存 icache 缓存中 */
int inode_load(struct inode *pi)
{
	if (pi->valid == 0)
	{
		struct cache_block *bbuf;
		if ((bbuf = cache_block_get(INODE_NUM(pi->inum, superblock))) == NULL) // 读取对应的inode记录所在的逻辑块
			return -1;

		return 0;
	}
	return 0;
}
