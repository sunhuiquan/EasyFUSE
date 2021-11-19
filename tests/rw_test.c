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
	int fd, readn;
	char ch;

	if ((fd = open(PATH, O_CREAT | O_RDWR | O_EXCL, 0666)) == -1)
		err_exit("open");

	// if (write(fd, "a", 1) != 1)
	// 	err_exit("write");
	if ((readn = read(fd, &ch, 1)) == -1)
		err_exit("read");
	if (readn == 0)
		printf("EOF\n");
	else
		printf("%c\n", ch);

	return 0;
}
