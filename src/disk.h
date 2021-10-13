#ifndef DISK_H
#define DISK_H

#include "fs.h"

/* 初始化磁盘 */
void init_disk(const char *path);

/* 从磁盘中读取物理块到内存中的block cache数组缓冲结构 */
struct cache_block *block_get(int blockno);

#endif