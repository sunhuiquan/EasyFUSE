// int main()
// {
// 	struct stat sbuf;

// 	if ((fd = open("diskimg", O_RDWR)) == -1)
// 		err_exit("open");

// 	// 注意我们对特定文件进行的系统调用，linux会通过他们的类型
// 	// 走向实际的对应处理代码，如果是ext4就走向ext4，如果是fuse
// 	// 就走向fuse的处理代码，如果是设备文件就走向对应的驱动，所以
// 	// 说我们这里对这个模拟磁盘文件走的代码实际是ex4，而之后对fuse
// 	// 挂载的系统的调用走向的才是我们实现libfuse接口实际的代码,
// 	// 所以虽然是同一个系统调用，但是执行的代码不同类型文件是不同的
// 	if (fstat(fd, &sbuf) == -1)
// 		err_exit("fstat");
// 	printf("%ld\n", (long)sbuf.st_size);

// 	// test
// 	// if (block_write("a", 1, 20) == -1)
// 	// 	err_exit("block_write");

// 	// to do 磁盘全部清零

// 	// to do 初始化超级块（读取，初始化，写回）
// }