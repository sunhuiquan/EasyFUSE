/* 声明该FUSE的系统调用 */

#ifndef USERSPACE_FS_CALLS_H
#define USERSPACE_FS_CALLS_H

#include <fuse.h>
#include <sys/types.h>
#include <sys/stat.h>

/* 开机系统启动后加载文件系统 */
void *userspace_fs_init();

/* wraper */
int wraper(const char *path, short type);

/* 获取文件属性，返回-ENOENT代表无此文件，注意要直接返回给libfuse接口用于通知 */
int userspace_fs_stat(const char *path, struct stat *sbuf);

/* 读取path目录的目录项 */
int userspace_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						 off_t offset, struct fuse_file_info *fi);

/* mkdir命令创建目录 */
int userspace_fs_mkdir(const char *path, mode_t mode);

/* rmdir命令删除目录 */
int userspace_fs_rmdir(const char *path);

#endif
