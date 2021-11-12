/* 声明该FUSE的系统调用 */

#ifndef USERSPACE_FS_CALLS_H
#define USERSPACE_FS_CALLS_H

#include <fuse.h>
#include <sys/types.h>
#include <sys/stat.h>

/* 开机系统启动后加载文件系统 */
void *userspace_fs_init();

/* wraper */
int wraper(char *path, ushort type);

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type);

/* 获取文件属性，注意该FS没有实现权限检测，所以显示是0777权限 */
int userspace_fs_stat(const char *path, struct stat *sbuf);

/* 读取path目录的目录项 */
int userspace_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
								off_t offset, struct fuse_file_info *fi);

#endif
