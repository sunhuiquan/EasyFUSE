/* 用于输出各种辅助信息，以便debug的辅助函数库 */

#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int min(int a, int b)
{
	return a < b ? a : b;
}

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}
