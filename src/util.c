/* 用于输出各种辅助信息，以便debug的辅助函数库 */

#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

int min(int a, int b)
{
	return a < b ? a : b;
}

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

/* 打印 superblock 的信息 */
void pr_superblock_information(const struct super_block *superblock)
{
	printf("super block:\n");
	printf("\t log_block_startno: %u\n", superblock->log_block_startno);
	printf("\t log_block_num: %u\n", superblock->log_block_num);
	printf("\t inode_block_startno: %u\n", superblock->inode_block_startno);
	printf("\t inode_block_num: %u\n", superblock->inode_block_num);
	printf("\t bitmap_block_startno: %u\n", superblock->bitmap_block_startno);
	printf("\t bitmap_block_num: %u\n", superblock->bitmap_block_num);
	printf("\t data_block_startno: %u\n", superblock->data_block_startno);
	printf("\t data_block_num: %u\n", superblock->data_block_num);
}

void pr(const char *msg)
{
	printf("%s\n", msg);
}

/* 出现致命错误，退出程序 */
void panic(const char *msg)
{
	printf("panic %s\n", msg);
	exit(EXIT_FAILURE);
}

void pr_open_flags(const struct fuse_file_info *fi)
{
	printf("flags: ");
	if (fi->flags & O_CREAT)
		printf("O_CREAT ");
	if (fi->flags & O_EXCL)
		printf("O_EXCL ");
	if (fi->flags & O_WRONLY)
		printf("O_WRONLY ");
	if (fi->flags == O_RDONLY) // RDONLY一般就自己但用，不过这里可能是因为未初始化也是0，而不是因为O_RDONLY
		printf("O_RDONLY（可能是因为未初始化也是0，而不是因为O_RDONLY） ");
	if (fi->flags & O_RDWR)
		printf("O_RDWR ");
	if (fi->flags & O_APPEND)
		printf("O_APPEND ");
	printf("\n");
}