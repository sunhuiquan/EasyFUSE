#ifndef FUSE_FS_STAT_H
#define FUSE_FS_STAT_H

// #include <sys/types.h>

// struct fuse_fs_stat
// {
// 	int dev;	 // 文件的设备类型
// 	uint ino;	 // inode号
// 	short type;	 // 文件类型
// 	short nlink; // 硬链接数
// 	uint size;	 // 文件大小
// };

#include <sys/stat.h>

int getattr(const char *path, struct stat *sbuf);

#endif
