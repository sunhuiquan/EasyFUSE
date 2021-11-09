/* 实现 libfuse open API */

#ifndef FUSE_FS_CALLS_H
#define FUSE_FS_CALLS_H

#include <sys/types.h>
#include <sys/stat.h>

/* 开机系统启动后加载文件系统 */
void *fuse_fs_init(void);

/* wraper */
int wraper(char *path, ushort type);

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type);

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
int fuse_fs_stat(const char *path, struct stat *sbuf);

#endif
