/* 磁盘驱动层，里面是读写磁盘的驱动程序（我们的这里是通过linux文件API的模拟），也是逻辑块号转物理块号被驱动程序
 * 使用的地方（不过我们的实现使用的逻辑块与物理块是一对一的关系）。
 */

#include "disk.h"
#include "util.h"
#include "inode.h"
#include "block_cache.h"
#include "log.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* 注意在我们的文件系统中的所有代码，使用的都是逻辑块，block_get()参数是逻辑块号，
 * block_put()参数中指定是逻辑块在内存中的结构，而涉及到逻辑块和物理块的映射的代码，
 * 只存在于驱动，那里会把我们的逻辑块自动映射成对应的物理块再由驱动代码使用，所以物理块
 * 对于FS代码是不可见的(和虚拟内存一样）。
 */

static int block_zero(int blockno);

#define MAGIC_NUMBER 0x123456789a

/* 模拟磁盘文件的file descriptor */
int disk_fd;

/* 这个内容在文件系统执行的时候不会更改(是关于磁盘布局的一些信息，如果磁盘没改变那么
 * 这个当然没有改变)，所以一直加载在内存作为一个全局变量，不需要写回(因为没改变过)
 */
struct super_block superblock;

/* 运行fuse，加载磁盘，读取必要信息 */
int load_disk(const char *path)
{
	if ((disk_fd = open(path, O_RDWR)) == -1)
		return -1;
	//这里的 superblock 加载在内存作为一个全局变量，和bcache缓冲无关，其他程序直接通过这个全局变量来访问
	if (lseek(disk_fd, 1 * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (read(disk_fd, &superblock, sizeof(struct super_block)) != sizeof(struct super_block))
		return -1;

	// 验证错误，不是我们的FUSE或者没有经过初始化磁盘init_disk程序
	if (superblock.magic != MAGIC_NUMBER)
	{
		errno = EINVAL;
		return -1;
	}

	// 输出加载的 superblock 的信息
	pr_superblock_information(&superblock);
	return 0;
}

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用不命中的结果），
 * 这里涉及到逻辑块和物理块的映射，不过我们的实现是一对一所以这里的代码体现不出来，
 * 另外这里的这个cache_block是持有锁的。
 */
int disk_read(struct cache_block *buf)
{
	if (lseek(disk_fd, buf->blockno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (read(disk_fd, &buf->data, BLOCK_SIZE) != BLOCK_SIZE)
		return -1;
	return 0;
}

/* 把内存块的内容写到磁盘上，这里涉及到逻辑块和物理块的映射，不过我们的实现是一对一所以这里的代码体现不出来
 * 另外这里的这个cache_block是持有锁的。
 */
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
 *
 * 必须在初始化日志层之前使用，内部直接read和write，不通过日志层。
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

// int balloc() 通过 bitmap 获取一个空的磁盘上的数据块号，清空磁盘上这个数据块的残余数据，设置 bitmap 对应位为1
int balloc()
{
	struct cache_block *bbuf;
	int k;

	for (int i = 0; i < superblock.bitmap_block_num; ++i) // bitmap块号，共32个
	{
		if ((bbuf = block_read(superblock.bitmap_block_startno + i)) == NULL)
			return -1;
		for (int j = 0; j < BLOCK_SIZE; ++j)
		{
			// 注意一定是无符号数右移，因为有符号数的>>右移会保持符号位不变
			unsigned char bit = bbuf->data[j];
			for (k = 0; k < 8; bit >>= 1, ++k)
				if (bit & 1 == 0)
				{
					bbuf->data[j] &= (1 << k); // 设置对应位图位为1
					if (write_log_head(bbuf) == -1)
					{
						block_unlock_then_reduce_ref(bbuf);
						return -1;
					}
					if (block_unlock_then_reduce_ref(bbuf) == -1)
						return -1;
					if (block_zero(j * 8 + k) == -1)
						return -1;
					return (j * 8) + k;
				}
		}
		if (block_unlock_then_reduce_ref(bbuf) == -1)
			return -1;
	}
	return -1; // 无空闲数据块
}

/* 清空这个逻辑块号对应的磁盘上的物理块号（我们的对应关系是逻辑块号等于物理块号）
 *
 * 需要注意init_log之后就不要直接使用disk_write了，因为disk_write可不管log层的锁机制，导致竞争错误。
 */
static int block_zero(int blockno)
{
	struct cache_block *bbuf;
	if ((bbuf = block_read(blockno)) == NULL)
		return -1;
	memset(bbuf->data, 0, BLOCK_SIZE);
	if (write_log_head(bbuf) == -1)
	{
		block_unlock_then_reduce_ref(bbuf);
		return -1;
	}
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
	return 0;
}

/* 释放磁盘上的数据块，单纯就是把位图设置成0，不清零，因为balloc()分配新数据块的时候会初始化清零
 *
 * 这个是在日志层之后使用，所以不能使用bitmap_set_or_clear，需要考虑日志操作。
 */
int block_free(int blockno)
{
	struct cache_block *bbuf;
	uint a = blockno / BIT_NUM_BLOCK;
	blockno %= BIT_NUM_BLOCK;
	uint b = blockno / 8;
	uint c = blockno % 8;

	if ((bbuf = block_read(superblock.bitmap_block_startno + a)) == NULL)
		return -1;
	bbuf->data[b] &= ~(1 << c);
	if (write_log_head(bbuf) == -1)
	{
		block_unlock_then_reduce_ref(bbuf);
		return -1;
	}
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
}
