#ifndef DISK_H
#define DISK_H

#include <unistd.h>
#include "fs.h"
#include "util.h"

static int disk_fd;

/* 初始化磁盘 */
void init_disk(const char *path)
{
}

/* 读取物理块进入内存，放入空闲的 block cache 中 */
int block_get(uint no) // 逻辑块号
{
	struct cache_block buf;
	off_t offset = no * BLOCK_SIZE;
	if(read(disk_fd,buf.data,BLOCK_SIZE) != BLOCK_SIZE)
		err_exit("read");
	// 这里的read是读肯定有内容的文件，不错误的情况下一定是返回要求我们读的大小
}

#endif