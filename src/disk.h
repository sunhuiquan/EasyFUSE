#ifndef DISK_H
#define DISK_H

#include "fs.h"
#include "block_cache.h"

#define BIT_NUM_BLOCK 8192 // BLOCKSIZE * 8

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果） */
int disk_read(struct cache_block *buf);

/* 把内存块的内容写到磁盘上 */
int disk_write(struct cache_block *buf);

/* 设置或清除对于磁盘块的bitmap位标志 */
int bitmap_set_or_clear(int blockno, int is_set);

#endif
