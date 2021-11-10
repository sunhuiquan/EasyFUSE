#ifndef FS_H
#define FS_H

#include <sys/types.h> // 为了用 uint（unsigned int），因为方便

#define DISK_FILE_PATH "./diskimg"

#define BLOCK_SIZE 1024 // 块大小 1024B

#define FILE_DIR 1
#define FILE_REG 2
#define FILE_LNK 4

/* log block的数目，代表着一个事务最多能一次写多少个块(要-1因为log header块)。
 *
 * 大部分的写磁盘的系统调用要写入的磁盘块数很少，但是我们使用的这个写事务是批处理的，可以让多个进程的写系统调用
 * 放在一个事务里面，一次写入磁盘来提高效率，因此可以用到多个日志块，受这个块数的限制。
 *
 * 另一种情况是如write这个系统调用，当写一个大文件的时候，如果写入非常多的数据，那么会需要大量的log block，可能
 * 当前这个日志块不够用，所以write如果需要的情况需要切割成多个写，因为超过最大容量的话是等不到够用的时候的。
 *
 * 一些如unlink的操作，可能需要写多个bitmap块，不过我们这里保证了bitmap的块数量比MAX_WRITE_SYSCALLS小，所以不用考虑切分，
 * 如果当前的日志块不够用的时候unlink会等到够用，而肯定有这个时候的。
 */
#define MAX_WRITE_SYSCALLS 35 // 假设一个系统调用最多写的块数，这个会限制一个write最大的写使用块数
#define LOG_BLOCK_NUM (3 * MAX_WRITE_SYSCALLS)

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

// 返回路径文件所在的目录的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_dir_inode(const char *path, char *name);

// 返回路径对应文件的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_path_inode(const char *path, char *name);

/* 通过目录 inode 查找该目录下对应 name 的 inode 并返回，通过iget，返回未加锁，且已增加了引用计数。
 * (内存中调用iget就说明有新的对象指向了这个inode缓存，调用iget就会导致引用计数+1)
 */
struct inode *dir_find(struct inode *pdi, char *name);

/* 获取当前层(比如中间的路径名，直到最后的文件名)的路径名 */
const char *current_dir_name(const char *path, char *name);

// 将name和inum组合成一条dirent结构，写入pdi这个目标inode结构，注意pdi是持有锁的
int add_dirent_entry(struct inode *pdi, char *name, uint inum);

#endif
