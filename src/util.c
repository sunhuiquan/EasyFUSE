#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

void test_err_exit(int lineno, const char *msg)
{
	printf("%s failed: %s on line %d\n", msg, strerror(errno), lineno);
	exit(EXIT_FAILURE);
}
