#include "fuse_fs_calls.h"
#include "fs.h"

/* 开机系统启动后加载文件系统 */
void *fuse_fs_init(void)
{
	if (load_disk(DISK_FILE_PATH) == -1) // 加载磁盘，读取superblock到内存的全局变量中
		err_exit("load_disk(argv[1])");
	if (inode_cache_init() == -1) // 初始化 inode cache
		err_exit("inode_cache_init");
	if (init_block_cache_block() == -1) // 初始化 block cache
		err_exit("init_block_cache_block");
	if (init_log() == -1) // 初始化log，并进行磁盘恢复操作
		err_exit("init_log");
}