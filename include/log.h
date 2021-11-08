#ifndef LOG_H
#define LOG_H

/* 初始化日志层，这是内核加载文件系统，文件系统初始化的一部分 */
int init_log();

/* 提供的实际写入磁盘的接口函数，把要写的数据块的块号放入log head数组，确立一个空闲的普通日志块与之对应 */
int write_log_head(struct cache_block *bbuf);

/* 进入事务（多个进程的begin_op可以让多个系统调用放到一个事务里面） */
int in_transaction();

/* 离开事务，如果离开时无系统调用处于事务中，则提交事务 */
int out_transaction();

#endif
