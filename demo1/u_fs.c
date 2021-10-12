/*
 FUSE: Filesystem in Userspace
 Copyright (C) 2016  Nango <luonango@qq.com>

 This program can be distributed under the terms of the GNU GPL.
 See the file COPYING.
 */
/***
 * author: nango
 * using:
 * start: 2016-02-10 20:00
 * finished:
 *
 *
 *
 ***/

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include "global.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

/**
 * This function should write the data in buf into the file denoted by path
 * staring at offset.
 *
 * return size on success(file size)
 * return -EFBIG if the offset is beyond the file size(but handle appends)
 */
static int ufs_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi);

/**
 * This function should delete an empty directory.
 *
 * return 0 on success
 * return -ENOTEMPTY if the directory is not empty
 * return -ENOENT if the directory is not found
 * return -ENOTDIR if the path is not a directory.
 */
static int ufs_rmdir(const char *path);

/**
 * This function should add a new file to a sub-directory
 * and should update the directories entry structure.
 *
 * return 0 on success.
 * return -ENAMETOOLONG if the name is beyond 8.3 chars
 * return -EPERM if the file is trying to be created in the root directory
 * return -EEXIST if the file already exists
 *
 */
static int ufs_mknod(const char *path, mode_t mode, dev_t rdev);

/**
 * This function should delete a file with path.
 * return 0 on success
 * return -EISDIR if path is a directory
 * return -ENOENT if the file is not found
 */
static int ufs_unlink(const char *path);

/**
 *  This function should not be modified.
 */
static int ufs_truncate(const char *path, off_t size);

/**
 *  This function should not be modified.
 */
static int ufs_flush(const char *path, struct fuse_file_info *fi);

/**
 * This function should add the new directory to the root level
 * and should update the directories file.
 *
 * return -ENAMETOOLONG if the name is beyond 8 chars
 * return -EPERM if the directory is not under the root dir only
 * return -EEXIST if the directory already exists
 *
 */
static int ufs_mkdir(const char *path, mode_t mode);

/**
 * This function should look up the input path.
 * ensuring that it's a directory, and then list the contents.
 * return 0 on success
 */
static int ufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
					   off_t offset, struct fuse_file_info *fi);

/**
 * This function should not be modified,
 * as you get the full path every time any of the other functions are called.
 * return 0 on success
 */
static int ufs_open(const char *path, struct fuse_file_info *fi);

/**
 * This function should read the data in the file denoted by path into buf,
 * starting at offset.
 *
 * return size read on success
 * return -RISDIR if the path is a directory
 */
static int ufs_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi);

/**
 * This function look up the input path to determine if it is a directory or a file.
 * if it is a directory , return the appropriate permissions.
 * if it is a file ,return the appropriate permissions as well as the actual size.
 * This size must be accurate since it is used to determine EOF
 *    and thus read may not be called.
 *
 * return 0 on success,with a correctly set structure
 * return -ENOENT if the file is not found.
 */
static int ufs_getattr(const char *path, struct stat *stbuf);

/**
 * This function initialize the filesystem when run,
 * read the supper block and
 * set the global variables of the program.
 */
void *u_fs_init(struct fuse_conn_info *conn)
{
	utils_init();
	return (long *)TOTAL_BLOCK_NUM;
}

//*****************************************************************//

static struct fuse_operations u_operation = {
	.getattr = ufs_getattr, .open = ufs_open, .readdir = ufs_readdir, .mkdir = ufs_mkdir, .rmdir = ufs_rmdir, .mknod = ufs_mknod, .write = ufs_write, .read = ufs_read, .unlink = ufs_unlink, .truncate = ufs_truncate, .flush = ufs_flush, .init = u_fs_init};

//***************************************************//
int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &u_operation, NULL);
}
//***************************************************//

static int ufs_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi)
{
	return utils_write(path, buf, size, offset);
}

static int ufs_rmdir(const char *path)
{
	return utils_rmdir(path);
}

static int ufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	return utils_mknod(path, mode, rdev);
}

static int ufs_unlink(const char *path)
{
	return utils_unlink(path);
}

static int ufs_truncate(const char *path, off_t size)
{
	return utils_truncate(path, size);
}

static int ufs_flush(const char *path, struct fuse_file_info *fi)
{
	return utils_flush(path, fi);
}

static int ufs_mkdir(const char *path, mode_t mode)
{
	return utils_mkdir(path, 2);
}

static int ufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
					   off_t offset, struct fuse_file_info *fi)
{
	return utils_readdir(path, buf, filler, offset);
}

static int ufs_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int ufs_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	return utils_read(path, buf, size, offset);
}

static int ufs_getattr(const char *path, struct stat *stbuf)
{
	return utils_getattr(path, stbuf);
}
