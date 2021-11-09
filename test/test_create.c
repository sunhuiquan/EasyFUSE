#include "fuse_fs_calls.h"
#include "util.h"
#include "inode.h"
#include "disk.h"
#include "inode_cache.h"
#include "block_cache.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (wraper("/a/", FILE_DIR) == -1) // 创建 /a/ 目录文件测试
		err_exit("create(\"/a/\", FILE_DIR)");

	/* 非自动化测试，需要自己手动分析磁盘块得知结果是否正确 */
	printf("create 测试通过（但该测试只检测是否执行错误，需要手动分析磁盘块得知是否实际结果正确）\n");
	return 0;
}
