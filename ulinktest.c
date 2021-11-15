#include <stdio.h>
#include <unistd.h>

int main()
{
	if (unlink("./build/mountdir/a") == -1)
		printf("wrong\n");
	return 0;
}