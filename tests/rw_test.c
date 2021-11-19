#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PATH "../build/mountdir/b"

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

int main()
{
	int fd1;
	char ch;

	if ((fd1 = open(PATH, O_RDWR)) == -1)
		err_exit("fd1 open");

	for (int i = 0; i < 5000; ++i)
		if (write(fd1, "a", 1) != 1)
			err_exit("fd1 write");

	int readn;
	if ((readn = read(fd1, &ch, 1)) == -1)
		err_exit("fd1 read");

	if (readn == 0)
		printf("EOF\n");
	else
		printf("%c\n", ch);
	return 0;
}
