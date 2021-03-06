#ifndef INODE_H
#define INODE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#define ROOT_INODE 0 // 根路径所在的inode号为0，即第一个inode

/* inode实体的文件类型(这里的文件概念是泛指，目录也是文件的一种) */

/* 一个物理块的大小是1024B，为了对齐且大小适中，我们采用的磁盘上的 struct disk_inode_block 大小为 64B */

/* inode块实际在磁盘物理块中存储的数据形式 */
#define NDIRECT 12
#define NINDIRECT 1

struct disk_inode
{
	short type;	 // inode对应的文件的类型(普通、目录、设备、链接)，0 代表未被分配过
	short major; // 如果是设备文件那么有major主设备号
	short minor; // 如果是设备文件那么有minor次设备号
	short nlink; // 硬链接数(打开文件描述指向inode)
	uint size;	 // size对于目录是分配数据块的大小，是BLOCK_SIZE的倍数，而对于文件才是真实的数据大小
	uint addrs[NDIRECT + NINDIRECT];
	// 指向的数据块(采用三级索引),addrs的元素数是disk_inode_block大小减去其他字段剩下的大小计算得来 (64 - (2 * 4) - 4) / 4 => 13
};

/**
 * inode块在内存中的存储形式，里面的dinode就是从磁盘物理块读出对应struct
 * disk_inode_block结构的磁盘上的数据，另外一些是在管理文件系统的辅助信息,
 * 在内存中的形式是inode cache，和之前实现的数据块cache原理一样。
 */
struct inode
{
	uint dev;					// 包含了主设备和次设备的信息，意思是可以用major()和minor()宏提取major和minor字段
	uint inum;					// inode号，实际是在磁盘物理块上的inode的位次（即磁盘数的第几个inode结构）
	int ref;					// inode缓存的引用计数
	int valid;					// 是否已加载入inode缓存
	pthread_mutex_t inode_lock; // pthread互斥锁对单个内存中的 inode 结构加锁

	struct disk_inode dinode; // inode的实际存储的数据信息(同样也是存在磁盘上的)
};

#endif
