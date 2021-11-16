#include "userspace_fs_calls.h"

/* 读符号链接，把指向的路径写到buf里面，注意这是对符号链接操作 */
int userspace_fs_readlink(const char *path, char *buf, size_t bufsz)
{
}
