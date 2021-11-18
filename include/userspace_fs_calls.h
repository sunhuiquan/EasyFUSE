/* 声明该FUSE的系统调用 */

#ifndef USERSPACE_FS_CALLS_H
#define USERSPACE_FS_CALLS_H

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <sys/types.h>
#include <sys/stat.h>

/* 开机系统启动后加载文件系统 */
void *userspace_fs_init();

/* 获取文件属性，返回-ENOENT代表无此文件，注意要直接返回给libfuse接口用于通知 */
int userspace_fs_stat(const char *path, struct stat *sbuf);

/* 读取path目录的目录项 */
int userspace_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						 off_t offset, struct fuse_file_info *fi);

/* mkdir命令创建目录 */
int userspace_fs_mkdir(const char *path, mode_t mode);

/* rmdir命令删除目录 */
int userspace_fs_rmdir(const char *path);

/* 实现 libfuse create 系统调用，可用于创建普通文件 */
int userspace_fs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

/* 删除目录项，降低nlink计数 */
int userspace_fs_unlink(const char *path);

/* 创建硬链接，注意不能给目录创建硬链接 */
int userspace_fs_link(const char *oldpath, const char *newpath);

/* 创建符号链接，可以为目录创建符号链接 */
int userspace_fs_symlink(const char *oldpath, const char *newpath);

/* 读符号链接，把指向的路径写到buf里面，注意这是对符号链接操作 */
int userspace_fs_readlink(const char *path, char *buf, size_t bufsz);

/* 实现 libfuse rename 系统调用 */
int userspace_fs_rename(const char *oldpath, const char *newpath);

/* 实现 libfuse open 系统调用 */
int userspace_fs_open(const char *path, struct fuse_file_info *fi);

/* 实现 libfuse read 系统调用 */
int userspace_fs_read(const char *path, char *buf, size_t bufsz,
					  off_t offset, struct fuse_file_info *fi);

/* 实现 libfuse write 系统调用 */
int userspace_fs_write(const char *path, const char *buf, size_t count,
					   off_t offset, struct fuse_file_info *fi);

/* 实现 libfuse truncate 系统调用 */
int userspace_fs_truncate(const char *path, off_t offset);

#endif
