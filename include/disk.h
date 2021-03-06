/* 磁盘驱动层，里面是读写磁盘的驱动程序（我们的这里是通过linux文件API的模拟），也是逻辑块号转物理块号被驱动程序
 * 使用的地方（不过我们的实现使用的逻辑块与物理块是一对一的关系）。
 */

#ifndef DISK_H
#define DISK_H

#include "fs.h"
#include "block_cache.h"

extern struct super_block superblock;

#define BIT_NUM_BLOCK (BLOCK_SIZE * 8) // 一个bitmap块标识的块数

/* 运行fuse，加载磁盘，读取必要信息 */
int load_disk(const char *path);

/* 把磁盘内容读到我们之前取到的空闲cache块中（cache_block_get()调用的结果） */
int disk_read(struct cache_block *buf);

/* 把内存块的内容写到磁盘上 */
int disk_write(struct cache_block *buf);

/* 设置或清除对于磁盘块的bitmap位标志 */
int bitmap_set_or_clear(int blockno, int is_set);

// int balloc() 通过 bitmap 获取一个空的磁盘上的数据块号，设置 bitmap 对应位为1
int balloc();

/* 释放磁盘上的数据块，单纯就是把位图设置成0，不清零，因为balloc()分配新数据块的时候会初始化清零 */
int block_free(int blockno);

#endif
