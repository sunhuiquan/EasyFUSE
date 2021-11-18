#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PATH "../build/mountdir/a"

void err_exit(const char *msg)
{
	printf("%s failed: %s\n", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

int main()
{
	int fd1, fd2;
	char ch;

	if ((fd1 = open(PATH, O_WRONLY)) == -1)
		err_exit("fd1 open");

	if (write(fd1, "b", 1) != 1)
		err_exit("fd1 write");

	if (write(fd1, "a", 1) != 1)
		err_exit("fd1 write");

	if (write(fd1, "a", 1) != 1)
		err_exit("fd1 write");

	if (write(fd1, "a", 1) != 1)
		err_exit("fd1 write");

	if ((fd2 = open(PATH, O_RDONLY)) == -1)
		err_exit("fd2 open");

	if (read(fd2, &ch, 1) != 1)
		err_exit("fd2 read");
	printf("%c\n", ch);

	if (read(fd2, &ch, 1) != 1)
		err_exit("fd2 read");
	printf("%c\n", ch);

	if (read(fd2, &ch, 1) != 1)
		err_exit("fd2 read");
	printf("%c\n", ch);

	if (read(fd2, &ch, 1) != 1)
		err_exit("fd2 read");
	printf("%c\n", ch);

	return 0;
}
