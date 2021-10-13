#ifndef FS_H
#define FS_H

#include <sys/types.h> // 为了用 uint（unsigned int），因为方便

#define BLOCK_SIZE 1024 // 块大小 1024B

// 我们的块号从 0 开始，另外 block 0 是 boot block，用不到
// super block 的块号是 1
struct super_block // super block 在内存中的数据结构
{
	uint block_num;		  // block 总数
	uint inode_block_num; // inode block 总数
	uint data_block_num;  // data  block 总数
	uint inode_block_startno;  // inode block 开始的块号
	uint data_block_startno;	  // data  block 开始的块号
};

struct cache_block // 缓存在内存的 cache block 结构
{
	uint refcnt;	 // 引用计数，到 0 的时候代表释放可重用
	uint is_changed; // 脏页标志
	struct cache_block *prev;
	struct cache_block *next;
	uint data[BLOCK_SIZE]; // 实际 block 的内容，即实际读入/写回磁盘的内容，前几个字段都是内存中才有的
};

struct inode_block // inode block 在内存中的数据结构
{
};

#endif
