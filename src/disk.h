#ifndef DISK_H
#define DISK_H

#include "fs.h"
#include "block_cache.h"

/* 初始化磁盘 */
int init_disk(const char *path);

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果） */
int block_read(struct cache_block *buf);

#endif