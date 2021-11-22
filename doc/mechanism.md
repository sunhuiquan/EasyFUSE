# 用户态、并发支持、日志恢复机制

   1. 再是对data block layer的锁机制，同样是两层锁机制，原理和inode layer锁机制原理一样。

   2. logging layer使用pthread mutex互斥锁，保证原子性的情况下，使用pthread cond条件变量避免轮询加解锁判断条件，提高性能。
   3. 避免死锁，注意我们内部的通过path查找inode的两个函数 find_dir_inode() 和 find_path_inode()，内部都是按照从根路径向末尾加锁的顺序来对每一级的目录的inode加锁，按照这个顺序，我们可以保证并发查找inode的时候不会发生死锁。