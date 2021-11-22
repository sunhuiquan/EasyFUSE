
   1. 避免死锁，注意我们内部的通过path查找inode的两个函数 find_dir_inode() 和 find_path_inode()，内部都是按照从根路径向末尾加锁的顺序来对每一级的目录的inode加锁，按照这个顺序，我们可以保证并发查找inode的时候不会发生死锁。