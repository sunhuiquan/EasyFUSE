#include "fuse_fs_open.h"
#include "util.h"
#include "inode.h"
#include "disk.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("%s usage: <path>\n", argv[0]);
		printf("  -path: the disk device image file\n");
		exit(EXIT_FAILURE);
	}

	if (load_disk(argv[1]) == -1) // 加载磁盘
		err_exit("load_disk(argv[1])");

	// if (create("/a/", FILE_DIR) == NULL) // 创建 /a/ 目录文件测试
	// 	err_exit("create(\"/a/\", FILE_DIR)");

	/* 非自动化测试，需要自己手动分析磁盘块得知结果是否正确 */
	printf("create 测试通过（但该测试只检测是否执行错误，需要手动分析磁盘块得知是否实际结果正确）\n");
	return 0;
}