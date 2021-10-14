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
	if (real_size <= 64 * BLOCK_SIZE)
		return -1;

	// 现在未加入日志支持，无logging layer的情况
	// 0          | 1           | 2 ...        | rest
	// boot block | super block | inode blocks | data blocks

	// to do
	// 初始化 super block 块
	superblock.block_num = real_size / BLOCK_SIZE; // 向下舍入
	superblock.inode_block_startno = 2;
	// superblock.data_block_startno = ?;
	// superblock.inode_block_num = ? ;
	// superblock.data_block_num =	 ? ;

	// 初始化磁盘设备，只初始化用到的那些物理块即可
	for (int i = 0; i < superblock.block_num; ++i)
	{
		if (read(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
		memset(temp, 0, BLOCK_SIZE);
		if (write(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			return -1;
	}
	return 0;
}

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果） */
int disk_read(struct cache_block *buf)
{
	if (lseek(disk_fd, buf->blockno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (read(disk_fd, &buf, BLOCK_SIZE) != BLOCK_SIZE)
		return -1;
	return 0;
}
