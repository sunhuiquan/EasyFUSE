#include "userspace_fs_calls.h"
#include "fs.h"
#include "disk.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

/* 开机系统启动后加载文件系统 */
void *userspace_fs_init()
{
	printf("文件系统开始加载\n");
	if (load_disk(DISK_FILE_PATH) == -1) // 加载磁盘，读取superblock到内存的全局变量中
		err_exit("load_disk(argv[1])");
	if (inode_cache_init() == -1) // 初始化 inode cache
		err_exit("inode_cache_init");
	if (init_block_cache_block() == -1) // 初始化 block cache
		err_exit("init_block_cache_block");
	if (init_log() == -1) // 初始化log，并进行磁盘恢复操作
		err_exit("init_log");
	printf("文件系统加载结束\n");
	return NULL; // 我们用不到那个private_data字段，返回NULL即可
}
