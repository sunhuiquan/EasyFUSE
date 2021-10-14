#include "disk.h"
#include "util.h"
#include "block_cache.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

/**
 * 注意在我们的文件系统中的所有代码，使用的都是逻辑块，block_get()参数是逻辑块号，
 * block_put()参数中指定是逻辑块在内存中的结构，而涉及到逻辑块和物理块的映射的代码，
 * 只存在于驱动，那里会把我们的逻辑块自动映射成对应的物理块再由驱动代码使用，所以物理块
 * 对于FS代码是不可见的(和虚拟内存一样）。
 */

static int disk_fd;
static struct super_block superblock;

/* 初始化磁盘 */
int init_disk(const char *path)
{
	struct stat sbuf;
	uint real_size;
	char temp[BLOCK_SIZE];

	if ((disk_fd = open("diskimg", O_RDWR)) == -1)
		return -1;

	if (fstat(disk_fd, &sbuf) == -1)
		return -1;
	real_size = sbuf.st_size;
	if (real_size <= 256 * BLOCK_SIZE) // 避免过小导致下面的计数舍入出错，256个块是随便取一个最小值
		return -1;

	/* 现在未加入日志支持，无logging layer的情况
	 * 0          | 1           | 2 ...        | 2+inode块数  | 34+block块数 ...
	 * boot block | super block | inode blocks | bitmap blocks | data blocks
	 *
	 * 注意我们的设计的逻辑块是从0开始计数的，这很重要。
	 */
	// 初始化 super block 块
	superblock.block_num = real_size / BLOCK_SIZE; // 向下舍入

	/* 分析 inode block 和 data blocks 块数的比例:
	 * 首先知道一个 inode block 指的是用于放 inode 结构的一个 block，是 BLOCK_SIZE 大小，
	 * 而一个 inode条目结构 我们实现是 64 字节大小，实现是直接+间接+二重映射，一个inode
	 * 条目可以对应 1 ~ 267(11 + 256) 个数据块（见inode结构可看懂），但文件系统中小文件才是
	 * 占绝对比例的，一般 99% 以上都是小文件，所以我们设计的inode条目对应block的比例是一对一，
	 * 注意是 inode 条目，而一个 inode block 可以有 BLOCK_SIZE / INODE_SIZE为 1024/64 是
	 * 16个，所以 inode block 和数据块比例是1比16。
	 * 这个分析是假定全是小文件，假定 inode 所需要最多的情况。
	 */
	superblock.inode_block_startno = 2;
	superblock.inode_block_num = (superblock.block_num - 34) / 17; // 1 : 16 的比例

	superblock.bitmap_block_startno = 2 + superblock.inode_block_num;
	superblock.data_block_startno = 32 + superblock.bitmap_block_startno;
	superblock.data_block_num = superblock.block_num - 34 - superblock.inode_block_num;

	/* 和 ext4 不同的是，ext4 使用块组的概念，同时一个块组只有一个 bitmap 块，而我们的文件系统
	 * 不涉及块组的概念，所以 bitmap 块需要多一些才能让文件系统支持的足够大。
	 *
	 * 另外 bitmap block 一个位对应一个块，能表示 1024 * 8 = 8192 个块，只有8192 * 1KB，
	 * 即整个文件系统最多能放 8MB 的数据(这里把非数据块也算上了)，显然太小，所以我们多使用几个
	 * bitmap块，我们这里使用了32个 bitmap block，能表示 256MB 的文件系统。
	 */

	// 初始化磁盘设备，只初始化用到的那些物理块即可
	for (int i = 0; i < superblock.block_num; ++i)
	{
		if (read(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
		memset(temp, 0, BLOCK_SIZE);
		if (write(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
	}

	// 初始化bitmap块
	char buf[BLOCK_SIZE];
	// to do
	if (write(disk_fd, superblock.bitmap_block_startno * BLOCK_SIZE, BLOCK_SIZE) != BLOCK_SIZE)
		return -1;

	return 0;
}

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果），
 * 这里涉及到逻辑块和物理块的映射，不过我们的实现是一对一所以这里的代码体现不出来
 */
int disk_read(struct cache_block *buf)
{
	if (lseek(disk_fd, buf->blockno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (read(disk_fd, &buf, BLOCK_SIZE) != BLOCK_SIZE)
		return -1;
	return 0;
}

/* 把内存块的内容写到磁盘上，这里涉及到逻辑块和物理块的映射，不过我们的实现是一对一所以这里的代码体现不出来 */
int disk_write(struct cache_block *buf)
{
	if (lseek(disk_fd, buf->blockno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (write(disk_fd, buf->data, BLOCK_SIZE) != BLOCK_SIZE)
		return -1;
}
