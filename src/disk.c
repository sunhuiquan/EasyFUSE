#include "disk.h"
#include "fs.h"
#include "util.h"
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
void init_disk(const char *path)
{
	struct stat sbuf;
	uint real_size;
	char temp[BLOCK_SIZE];

	if ((disk_fd = open("diskimg", O_RDWR)) == -1)
		err_exit("open");

	if (fstat(disk_fd, &sbuf) == -1)
		err_exit("fstat");
	real_size = sbuf.st_size;
	if (real_size <= 64 * BLOCK_SIZE)
		err_exit("diskimg too small");

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
			err_exit("clear disk");
		memset(temp, 0, BLOCK_SIZE);
		if (write(disk_fd, temp, BLOCK_SIZE) != BLOCK_SIZE)
			err_exit("clear disk");
	}
}

/* 从磁盘中读取物理块到内存中的block cache数组缓冲结构，注意这里的 
 * blockno 是物理块号，不过我们这个 FS 中的物理块号和逻辑块号是一
 * 对一的，不然需要分析逻辑块地址映射到具体的物理块 */
struct cache_block *block_get(int blockno)
{
}
