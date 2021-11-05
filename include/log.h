#ifndef LOG_H
#define LOG_H

/* 初始化日志层，这是内核加载文件系统，文件系统初始化的一部分 */
int init_log();

/* 提供的实际写入磁盘的接口函数，把要写的数据块的块号放入log head数组，确立一个空闲的普通日志块与之对应 */
int write_log_head(struct cache_block *bbuf)

#endif
