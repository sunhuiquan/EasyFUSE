/* rmdir命令删除目录 */
int userspace_fs_rmdir(const char *path)
{

	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	/* 我们这里只需要删除上一级目录的数据块中的该目录项，然后减少自己inode的硬链接引用计数，
	 * 然后直接完成，这里并不一定发生实际删除（综合取决于ref和nlink）。
	 *
	 * 之后的 inode_reduce_ref() 降低inode的引用（虽然不一定是在这里函数里面的inode_reduce_ref），
	 * 发现ref和nlink都为0，这就会具体地释放inode所占的数据块，这里就会把所持有的. .. 目录项清除，
	 * 注意rmidr命令需要目录项为空才可执行，-r需要递归调用unlink删除里面的目录项，实际删除文件。
	 */

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}