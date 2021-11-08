/* 实现 libfuse open API */

#ifndef FUSE_FS_CALLS_H
#define FUSE_FS_CALLS_H

#include <sys/types.h>

/* wraper */
int wraper(char *path, ushort type);

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type);

#endif
