#ifndef DISK_H
#define DISK_H

#include "fs.h"
#include "block_cache.h"

extern struct super_block superblock;

#define BIT_NUM_BLOCK 8192 // BLOCKSIZE * 8

/* 运行fuse fs文件系统，加载磁盘，读取必要信息 */
int load_disk(const char *path);

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果） */
int disk_read(struct cache_block *buf);

/* 把内存块的内容写到磁盘上 */
int disk_write(struct cache_block *buf);

/* 设置或清除对于磁盘块的bitmap位标志 */
int bitmap_set_or_clear(int blockno, int is_set);

// int balloc() 通过 bitmap 获取一个空的磁盘上的数据块号，设置 bitmap 对应位为1
int balloc();

#endif
