#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

#define BLOCK_SIZE 1024 // 块大小 1024B

// 我们的块号从 0 开始，另外 block 0 是 boot block，用不到
// super block 的块号是 1
struct super_block // super block 在内存中的数据结构
{
	uint fs_size;		  // 文件系统总大小
	uint inode_block_num; // inode block 总数
	uint data_block_num;  // data block 总数
	uint inode_start_no;  // inode block 开始的块号
	uint data_block_no;	  // data block 开始的块号
};

struct inode_block // inode block 在内存中的数据结构
{
};

static int fd;

/* 磁盘的一个扇区就是一个物理块，而一个物理块可以对应多个逻辑块
 * （即我们在文件系统中实际使用的逻辑块），物理块实际对我们不可见，
 * 由驱动程序完成逻辑块与物理块的映射，当然这里我们用设备文件抽象
 * 磁盘，这里物理块和逻辑块显然是一对一的，不用去管。
 */
// int block_write(const void *buf, size_t n, off_t offset)
// {
// 	if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
// 		return -1;
// 	if (write(fd, buf, n) != n)
// 		return -1;
// 	return 0;
// }

// int block_read()
// {
// }

int main()
{
	struct stat sbuf;

	if ((fd = open("diskimg", O_RDWR)) == -1)
		err_exit("open");

	// 注意我们对特定文件进行的系统调用，linux会通过他们的类型
	// 走向实际的对应处理代码，如果是ext4就走向ext4，如果是fuse
	// 就走向fuse的处理代码，如果是设备文件就走向对应的驱动，所以
	// 说我们这里对这个模拟磁盘文件走的代码实际是ex4，而之后对fuse
	// 挂载的系统的调用走向的才是我们实现libfuse接口实际的代码,
	// 所以虽然是同一个系统调用，但是执行的代码不同类型文件是不同的
	if (fstat(fd, &sbuf) == -1)
		err_exit("fstat");
	printf("%ld\n", (long)sbuf.st_size);

	// test
	// if (block_write("a", 1, 20) == -1)
	// 	err_exit("block_write");

	// to do 磁盘全部清零

	// to do 初始化超级块（读取，初始化，写回）
}