/*
 * utils.h
 *
 *  Created on: Feb 23, 2016
 *      Author: hadoop
 */

#ifndef  UTILS_H
#define  UTILS_H

#include "global.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/********************************************************************************/
/***************************隔离层，此处负责提供给文件系统的接口，由ufs调用***************/
/********************************************************************************/
void utils_init();

int utils_rmdir(const char *path);

int utils_mknod(const char *path, mode_t mode, dev_t rdev);

int utils_unlink(const char *path);

int utils_truncate(const char *path, off_t size);

int utils_flush(const char*path, struct fuse_file_info *fi);

int utils_mkdir(const char *path, mode_t mode);

/**
 * This function should look up the input path.
 * ensuring that it's a directory, and then list the contents.
 * return 0 on success
 */
int utils_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset);

/**
 * 根据路径path找到文件起始位置，再偏移offset长度开始读取数据到buf中，返回文件大小
 * return size
 * param
 * path：路径
 * buf：存储path对应文件信息
 * size：文件大小
 * offset：读取时候的偏移量
 * fi：fuse的文件信息
 */
int utils_read(const char *path, char *buf, size_t size, off_t offset);

/**
 * 通过路径path获得文件的信息，赋值给stat stbuf
 * return 0 mean success
 * param
 * path：文件路径
 * stbuf：stat文件信息
 */
int utils_getattr(const char *path, struct stat *stbuf);

/**
 * Write size bytes from buf into file starting from offset
 * 将buf里大小为size的内容，写入path指定的起始块后的第offset
 * （就像从指定行的第几个字符开始写的意思）
 */
int utils_write(const char *path, const char *buf, size_t size, off_t offset);

/*********************************************************************************/
/*********************************************************************************/
/**
 * This function should be able to output the running status information into log_path file
 * in order to debug or view.
 *
 *  tag: 标志符
 *  format：字符串格式
 */
void fslog(const char* tag, const char* format, ...);

/**
 * This function should be release one or more pointers.
 *
 * ptr: The pointer to be released
 */
void freePtrs(void*ptr, ...);

/**
 * Open a path of the file or directory, read the attribute assigned to return_dirent
 * 读取path对应dirent（文件或目录）信息，并赋值到return_dirent中
 * return 0 on success
 * return -1 on failed
 *
 * path: 欲获取文件信息的文件路径
 * return_dirent: 存放文件信息
 *
 */
int get_attr_from_open_pathblock(const char * path, struct u_fs_file_directory *attr);

/**
 * According to the block number move the pointer, read the assignment to blk_info
 * 根据块号移动指针，读出内容赋值给blk_info
 * return 0 on success , -1 on failed
 *
 * blk_pos : 将要读取的块的块号
 * block :   存放读到的块的内容
 */
int get_blkinfo_from_read_blkpos(long blk_pos, struct u_fs_disk_block * blk_info);

/**
 * Find a num continuous free block, get free block start block number start_block,
 * return to find the number of consecutive idle blocks (maximum otherwise found)
 * 找到num个连续空闲块，得到空闲块区的起始块号start_block，返回找到的连续空闲块个数（否则返回找到的最大值）
 *
 * num: 要寻找的连续空闲块个数
 * start_blk: 找到的空闲块的起始号码
 */
int get_free_blocks(int num, long* start_blk);

/**
 * Blk_info will be written into the specified block number of disk blocks
 * 将blk_info内容写进指定块号的磁盘块
 * return 0 on success , -1 on failed
 *
 * blk : 将要写入的块的块号
 * block :   将写入磁盘块的内容
 */
int write_blkinfo_start_blk(long blk, struct u_fs_disk_block * blk_info);

/**
 * Create a file or directory path is &path.
 * Flag 1 files, 2 directory
 * 创建path的文件或目录，flag 为1表示文件，2为目录
 *
 * return 0 on success , -1 on failed
 */
int create_file_dir(const char* path, int flag);

/**
 * Remove a file or directory path is &path.
 * Flag 1 files, 2 directory
 * 删除path的文件或目录，flag 为1表示文件，2为目录
 *
 * return 0 on success , -1 on failed
 */
int  rm_file_dir(const char *path, int flag);

/**
 * To determine whether &path is an empty directory
 * 判断是否为空目录
 * return 0 on yes
 */
int is_empty_dir(const char *path);

/**
 * 通过分割路径，读取父目录的信息。p_dir_blk为父目录的第一块子文件的位置
 */
int dv_path(char*name, char*ext, const char*path, long*p_dir_blk, int flag);

/**
 * 遍历目录下的所有文件和目录，如果已存在同名文件或目录，返回
 * 未处理多个块的情况
 */
int exist_check(struct u_fs_file_directory *file_dir, char *p, char *q,
		int* offset, int* pos, int size, int flag);

/**
 * 为上层目录增加一个后续块
 *
 */
int enlarge_blk(long p_dir_blk, struct u_fs_file_directory *file_dir,
		struct u_fs_disk_block *blk_info, long *tmp, char*p, char*q, int flag);

/**
 * 在bitmap中标记块是否被使用
 *
 */
int utils_set_blk(long start_blk, int flag);

/**
 * 初始化要创建的文件或目录
 */
void init_file_dir(struct u_fs_file_directory *file_dir, char*name, char*ext,
		int flag);
/**
 * 初始化disk_block单元
 */
void new_emp_blk(struct u_fs_disk_block *blk_info);

/**
 * 判断该path中是否含有子目录或子文件
 */
int is_empty(const char* path);

/**
 * 将attr属性赋值给path所指的文件属性
 */
int utils_setattr(const char* path, struct u_fs_file_directory* attr);

/**
 * The src_ attribute assignment to dest_
 * 读出src_的属性到dest_
 */
void set_file_dir(struct u_fs_file_directory *dest_,
		struct u_fs_file_directory *src_);

/**
 * From the beginning of the next_block block,
 * initialize blk_info block and the continued block
 * 从next_blk起清空blk_info后续块
 */
void nextClear(long next_blk, struct u_fs_disk_block* blk_info);

/**
 * Find the position according to the starting blocks and offset
 * 根据起始块和偏移量寻找位置
 * return 0 on success , -1 on failed.
 *
 * offset： 偏移量
 * start_blk: 起始块
 */
int find_off_blk(long*start_blk, long*offset, struct u_fs_disk_block *blk_info);

#endif /*  UTILS_H_ */
