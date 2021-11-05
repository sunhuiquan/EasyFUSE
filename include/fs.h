#ifndef FS_H
#define FS_H

#include <sys/types.h> // 为了用 uint（unsigned int），因为方便

#define BLOCK_SIZE 1024 // 块大小 1024B

// 我们的块号从 0 开始，另外 block 0 是 boot block，用不到
// super block 的块号是 1
struct super_block // super block 在内存中的数据结构
{
	uint block_num;			   // block 总数
	uint log_block_num;		   // log block 总数
	uint inode_block_num;	   // inode block 总数
	uint data_block_num;	   // data  block 总数
	uint bitmap_block_num;	   // bitmap  block 总数
	uint log_block_startno;	   // log block 开始的块号
	uint inode_block_startno;  // inode block 开始的块号
	uint data_block_startno;   // data  block 开始的块号
	uint bitmap_block_startno; // bitmap block 块号
	ulong magic;			   // 魔数，用于验证使用的FS确实是经过初始化，且是这个FS而不是其他FS的验证数
};

#define MAX_NAME 14 // 路径名最大长度

struct dirent
{
	ushort inum;
	char name[MAX_NAME];
};

#endif
