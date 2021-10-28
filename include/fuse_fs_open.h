#ifndef FUSE_FS_OPEN_H
#define FUSE_FS_OPEN_H


#include <sys/types.h>
/* 最后给 libfuse API 使用的 open 接口函数 */




// 测试用
struct inode* create(char *path, ushort type);

#endif
