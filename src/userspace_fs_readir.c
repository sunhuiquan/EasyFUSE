#include "userspace_fs_calls.h"

/* 读取path目录的目录项，要检测该path必须是一个目录文件，参数里面的buf和filler用于添加目录项，fih和off_set用不到 */
static int userspace_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
								off_t offset, struct fuse_file_info *fi)
{
	return 0;
}