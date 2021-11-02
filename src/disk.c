#include "disk.h"
#include "util.h"
#include "inode.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

/* 注意在我们的文件系统中的所有代码，使用的都是逻辑块，block_get()参数是逻辑块号，
 * block_put()参数中指定是逻辑块在内存中的结构，而涉及到逻辑块和物理块的映射的代码，
 * 只存在于驱动，那里会把我们的逻辑块自动映射成对应的物理块再由驱动代码使用，所以物理块
 * 对于FS代码是不可见的(和虚拟内存一样）。
 */

/* 模拟磁盘文件的file descriptor */
int disk_fd;

/* 这个内容在文件系统执行的时候不会更改(是关于磁盘布局的一些信息，如果磁盘没改变那么
 * 这个当然没有改变)，所以一直加载在内存作为一个全局变量，不需要写回(因为没改变过)
 */
struct super_block superblock;

/* 运行fuse fs文件系统，加载磁盘，读取必要信息 */
int load_disk(const char *path)
{
	if ((disk_fd = open(path, O_RDWR)) == -1)
		return -1;
	// to do 注意这里的 superblock 加载在内存作为一个全局变量，和bcache缓冲无关，其他程序直接通过这个全局变量来访问

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
	if (read(disk_fd, &buf, BLOCK_SIZE) != BLOCK_SIZE)
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

// int balloc() 通过 bitmap 获取一个空的磁盘上的数据块号，设置 bitmap 对应位为1
int balloc()
{
	// struct cache_block *bbuf;
	// for (int i = 0; i < superblock.bitmap_block_num; ++i) // bitmap块号，共32个
	// {
	// 	if ((bbuf = block_read(superblock.bitmap_block_startno + i)) == NULL)
	// 		return -1;
	// 	for (int j = 0; j < BLOCK_SIZE; ++j)
	// 	{
	// 		char bit = bbuf->data[j];
	// 	}
	// }
}