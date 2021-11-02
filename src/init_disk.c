#include "fs.h"
#include "util.h"
#include "inode.h"
#include "disk.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

extern int disk_fd;

/* 格式化磁盘，这个操作只在第一次安装(不是启动)fuse fs需要，或是文件系统崩溃，重新格式化的时候需要使用 */
static int init_disk(const char *path)
{
	struct stat sbuf;
	uint real_size;
	char temp[BLOCK_SIZE];
	struct super_block superblock;

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

	/* 初始化bitmap块,初始化时除了数据块全部设置成1，因为只有数据块需要使用这个 */
	for (int i = 0; i < superblock.bitmap_block_startno; ++i)
		if (bitmap_set_or_clear(i, 1) == -1)
			return -1;

	// ======================================================================================
	// 4.写入根目录到磁盘，别忘了这里也要设置bitmap，用我们下面写的函数实现
	struct disk_inode dinode;
	dinode.type = FILE_DIR;
	dinode.size = BLOCK_SIZE;
	dinode.nlink = 1;
	dinode.addrs[0] = superblock.data_block_startno;
	if (lseek(disk_fd, superblock.inode_block_startno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (write(disk_fd, &dinode, sizeof(struct disk_inode)) != sizeof(struct disk_inode))
		return -1;
	if (bitmap_set_or_clear(superblock.data_block_startno, 1) == -1) // 设置对应数据块位图为1
		return -1;

	struct dirent dir;
	dir.inum = 0;
	strncpy(dir.name, ".", MAX_NAME);
	if (lseek(disk_fd, superblock.data_block_startno * BLOCK_SIZE, SEEK_SET) == (off_t)-1)
		return -1;
	if (write(disk_fd, &dir, sizeof(struct dirent)) != sizeof(struct dirent))
		return -1;
	strncpy(dir.name, "..", MAX_NAME);
	if (write(disk_fd, &dir, sizeof(struct dirent)) != sizeof(struct dirent))
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("%s usage: <path>\n", argv[0]);
		printf("  -path: the disk device image file\n");
		exit(EXIT_FAILURE);
	}

	if (init_disk(argv[1]) == -1)
		err_exit("init_disk");

	printf("init disk successfully.\n");
	return 0;
}
