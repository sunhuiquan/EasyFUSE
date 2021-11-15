/* 删除目录项，降低nlink计数 */
int userspace_fs_unlink(const char *path)
{
	/* 需要具体写磁盘，所以需要事务操作 */
	if (in_transaction() == -1) // 进入事务
		return -1;

	/* 我们这里只需要删除上一级目录的数据块中的该目录项，删除目录项记得要把name也清空，
	 * 因为inum为0并不能完全代表未使用的情况，这里有点设计失误，然后减少该文件的inode
	 * 的硬链接引用计数，然后成功返回，这里并不发生实际删除。
	 *
	 * 之后的 inode_reduce_ref() （包括该函数里面的这次调用）降低inode的引用
	 * 如果发现ref和nlink都为0，这就会具体地释放inode所占的数据块，位图设置为0。
	 */

	if (out_transaction() == -1) // 离开事务
		return -1;
	return 0;
}
