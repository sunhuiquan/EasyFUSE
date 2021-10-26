#include "disk.h"
#include "util.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

/* 注意在我们的文件系统中的所有代码，使用的都是逻辑块，block_get()参数是逻辑块号，
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

	if ((disk_fd = open(path, O_RDWR)) == -1)
		return -1;

	if (fstat(disk_fd, &sbuf) == -1)
		return -1;
	real_size = sbuf.st_size;
	if (real_size <= 256 * BLOCK_SIZE) // 避免过小导致下面的计数舍入出错，256个块是随便取一个最小值
		return -1;

	// ======================================================================================
	// 1.清零磁盘设备文件
	int n = real_size / BLOCK_SIZE;
	for (int i = 0; i < n; ++i)
	{
		if (read(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
		memset(temp, 0, BLOCK_SIZE);
		if (lseek(disk_fd, i * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
			return -1;
		if (write(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
	}

	// ======================================================================================
	// 2.设置superblock，最后写入磁盘

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

	/* 和 ext4 不同的是，ext4 使用块组的概念，同时一个块组只有一个 bitmap 块，而我们的文件系统
	 * 不涉及块组的概念，所以 bitmap 块需要多一些才能让文件系统支持的足够大。
	 *
	 * 另外 bitmap block 一个位对应一个块，能表示 1024 * 8 = 8192 个块，只有8192 * 1KB，
	 * 即整个文件系统最多能放 8MB 的数据(这里把非数据块也算上了)，显然太小，所以我们多使用几个
	 * bitmap块，我们这里使用了32个 bitmap block，能表示 256MB 的文件系统。
	 */

	superblock.inode_block_startno = 2;
	superblock.inode_block_num = (superblock.block_num - 34) / 17; // 1 : 16 的比例
	superblock.bitmap_block_startno = 2 + superblock.inode_block_num;
	superblock.bitmap_block_num = 32;
	superblock.data_block_startno = 32 + superblock.bitmap_block_startno;
	superblock.data_block_num = superblock.block_num - 34 - superblock.inode_block_num;

	if (lseek(disk_fd, 1 * BLOCK_SIZE, SEEK_SET) == -1)
		return -1;
	if (write(disk_fd, &superblock, sizeof(superblock)) != sizeof(superblock))
		return -1;

	// ======================================================================================
	// 3.设置bitmap块，我们的bitmap_set_or_clear实现是没有缓存中间层，直接读写磁盘的

	/* 初始化bitmap块,初始化时除了 boot block， super block， 32个 bitmap block 之外都是0，
	 * 而之前初始化磁盘的时候已经全部清0了，所以这里只需要设1。
	 */
	char bitmap_buf[BLOCK_SIZE];
	if (bitmap_set_or_clear(0, 1) == -1) // 设置boot块的bitmap位为1
		return -1;
	if (bitmap_set_or_clear(1, 1) == -1) // 设置superblock块的bitmap位为1
		return -1;
	for (int i = 0; i < 32; ++i)
		if (bitmap_set_or_clear(i + superblock.bitmap_block_startno, 1) == -1)
			return -1;

	// ======================================================================================
	// 4.写入根目录到磁盘，别忘了这里也要设置bitmap，用我们下面写的函数实现

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
	return 0;
}

/* 公式 blockno = 8192*a + b + c
 * a是32个bitmap_buf中的从0开始算的第几个bitmap块，b是一个buf中从0第几个字节，c是从低位(右)开始从0读的第
 * 几个。
 *
 * 注意非常重要的是，对于bitmap，为了方便我们直接读写磁盘设备，而没有中间层cache这32个bitmap块，因为
 * 实在是太麻烦了，不过只有查找空闲的磁盘块才用得到，也就是实际分配新的数据块的时候，而新建文件的频
 * 率并不是太多，可以接受这个效率(修改、查看文件(含目录文件等各种文件)都和这个无关)。
 */
int bitmap_set_or_clear(int blockno, int is_set)
{
	char buf;
	uint a = blockno / BIT_NUM_BLOCK;
	blockno %= BIT_NUM_BLOCK;
	uint b = blockno / 8;
	uint c = blockno % 8;

	if (lseek(disk_fd, BLOCK_SIZE * (superblock.bitmap_block_startno + a) + b, SEEK_SET) == -1)
		return -1;
	if (read(disk_fd, &buf, 1) != 1)
		return -1;
	if (is_set)
		buf |= (1 << c);
	else
		buf &= ~(1 << c);
	if (lseek(disk_fd, BLOCK_SIZE * (superblock.bitmap_block_startno + a) + b, SEEK_SET) == -1)
		return -1;
	if (write(disk_fd, &buf, 1) != 1)
		return -1;
	return 0;
}
