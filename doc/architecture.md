# EasyFUSE 的系统架构

该文档用于详细描述 EasyFUSE 的系统架构，包括运行原理、分层结构、代码结构等的说明。

## 1. EasyFUSE 运行原理

### (1) libfuse 实现用户态的原理

各种形式的文件系统操作最后都是转成文件系统调用，然后陷入内核，经过 Linux VFS 处理，对于挂载用户态文件系统的文件系统操作 libfuse 会把调用请求转发给我们的 EasyFUSE 接口实现，我们的接口实现内部会对 EasyFUSE 底层的各层数据结构进行操作，然后返回结果，最后再由 libfuse 转发到 VFS 再到文件系统调用的结果返回，最终完成对文件系统的操作。

![IMG](../resource/libfuse_work.png)

\[1\] [Linux VFS (Virtual file system)](https://en.wikipedia.org/wiki/Virtual_file_system) 为用户程序提供文件系统操作的统一接口，屏蔽不同文件系统的差异和操作细节。  
\[2\] EasyFUSE 底层设施，在其之上实现 libfuse 接口，里面主要是disk、inode、data block、log、bitmap 这五个层的机制的实现代码。  
\[3\] libfuse 转发 VFS 的请求会调用相应的 libfuse 接口的实现。  
\[4\] [libfuse](https://github.com/libfuse/libfuse) 是一个 Linux 支持的、开源的、转发 Linux VFS 与 FUSE 之间的请求和响应来实现用户态文件系统的 (FUSE) 的库。  

### (2) 进程视角下 libfuse 映射

1. 首先我们要知道文件描述符（File descriptor）与打开文件描述（Open file handle/entry）这两个数据结构的层次是在 Linux 抽象文件系统（VFS）机制之上的，也就是说打开不同文件系统而来的文件描述符和相同FS的打开得到的fd没有什么不同，打开文件描述也是一样，都是由 Linux 内核维护，文件描述符和打开文件描述和我们的 EasyFUSE 一点关系都没有。同样对于比文件描述符的更高层包装类型文件流 FILE*，内部包含着文件描述符，同样如此与 FUSE 一点关系都没有。  
2. 我们实现的是 high-lever libfuse 接口，libfuse API 使用路径作为参数，libfuse 改变了原来的 **Open file handle——Inode** 的映射关系，提供了 **Open file handle——路径——Inode** 的映射关系。
3. 下面是一个示例，两次 open 调用打开同一个文件，两个打开文件描述 A、C 分别指向一个打开文件描述，然后通过 dup() 调用让文件描述符 B 和A指向同一个打开文件描述a，然后两个打开文件描述a、b指向同一个inode，但这个inode是个伪实现，这是因为要经过 libfuse 映射，它们传递给 libfuse 调用的都是路径实参，然后 libfuse 接口实现中再把路径转为真实的 inode 结构。
![IMG](../resource/fd_and_handle.png)  
\[1\] to do
\[2\]
\[3\]

## 2. EasyFUSE 底层设施(分层结构)

![IMG](../resource/layers.png)

### (1) 磁盘层 [[/src/disk.c](../src/disk.c)]

1. 实际读写磁盘的驱动代码（EasyFUSE 这里是用文件模拟磁盘读写），驱动这里有逻辑块号与物理块号的映射关系（EasyFUSE实现是1对1），该层向日志层提供事务提交时来真正读写物理磁盘的API。
2. 物理磁盘存储格式结构：
![IMG](../resource/disk.png)

- boot block是加载OS kernel的地方，这里未使用
- super block 存储的是该物理磁盘分区的元数据，比如各层的块数和开始块号
- log blocks 第一个头日志块存储着一次事务的信息，比如要提交的日志块数；其他的是普通的日志块，用于存放实际数据
- inode blocks 和 data blocks 的块数按照一定比例根据磁盘大小自动计算
- bitmap blocks 使用固定32个块，这个值限定了FS最大的大小，我们这里为了简化未实现如ext4的块组的方式，只有一个块组，所以我们的位图块数需要多一些。  
- data blocks 是实际的数据块，里面存在文件的数据

### (2) 日志层 [[/src/log.c](../src/log.c)]

1. 写文件系统调用需要写入磁盘（或其他外存设备或者网络存储）上的内容，会改变外存存储的元数据和普通数据，而在软件崩溃（如OS内核崩溃）或者硬件崩溃（如断电）很有可能造成存储的元数据和普通数据的不一致问题，日志恢复的核心工作就是处理这一个数据不一致的问题，避免导致严重错误。
2. 事务批处理机制指的是写文件系统调用使用 in_transaction() 和 out_transaction() 进出事务，注意并不是一个写系统调用一个事务，而是一个事务里面可以包含多个写系统调用，当之后当日志块容量不足或者目前无写系统调用被调用的时候，一起提交写入日志。  
![IMG](../resource/transaction.png)  
通过进出事务内部的吸收机制，以及这种批处理提交的形式，减少实际磁盘读写次数，提高效率。  
3. to do 原理图与日志恢复的原理  
![IMG](../resource/log_work.png)

### (3) Inode层 [[/src/inode_cache.c](../src/inode_cache.c)]

1. 存储文件的元数据，比如设备号、文件类型、大小、硬链接数等信息，然后也存放着两级对实际存放着文件数据的磁盘数据块的索引，里面存放着对应的物理数据块的逻辑块号。
2. 磁盘上的Inode的结构 (对应 struct disk_inode 类型) 以及二级索引的实现
![IMG](../resource/disk_inode.png)

采用二级索引，一级索引有12个，二级索引指向一个数据块，然后可放256个索引，所以一共268个索引块，所以我们的 EasyFUSE 中，一个文件最大的大小是 268 * 1024B = 268KB，可以使用三级甚至四级页表来调高最大文件的大小。
3. 提供磁盘上的 inode 结构在内存上的 cache 机制，减少读取磁盘的加载的次数，提高效率，里面通过二层锁机制（对cache整体和每个内存中的inode分别加锁）实现并发性，通过引用计数来实现缓存里面的元素的释放与复用。  
第一层锁是对于 icache 整体加锁保证了一个 inode 在缓存只有一个，并且保证内存中的 inode 结构引用计数正确；第二层锁是对每个缓存中的 inode 结构都有一个锁，保证了可互斥访问 inode 的相关字段，以及 inode 所对应的数据块。
4. 该层写磁盘最终是借助日志层的 write_log_head() 调用，并最终在日志提交时才真正进行写磁盘。
5. Inode层通过 nlink 硬链接计数，内存中减少引用计数的调用（比如unlink、rmdir等），如果导致 nlink 减少为0，则会释放该磁盘中的inode结构，并释放对应磁盘上的数据块，位图对应设置为0。

### (4) 位图层 [[/src/disk.c](../src/disk.c)]

1. 位图块用于快速找到空闲 磁盘块并返回块号，一个位图块有8192个位，位上为0代表空闲，可以代表8192个磁盘块，这样我们只需要读最多32个位图块，而不会遍历读取非常多的磁盘块来找到空闲磁盘块，减少IO次数来提高效率。

### (5) 块缓存层 [[/src/block_cache.c](../src/block_cache.c)]

1. 块缓存层，提供磁盘上的块加载到内存的缓存机制，通过cache减少读取磁盘的次数，提高效率。
2. 和Inode层一样，通过两层锁机制和引用计数保证并发支持，以及正确的释放和复用缓存空间。
3. 该层写磁盘最终是借助日志层的 write_log_head() 调用，并最终在日志提交时才真正进行写磁盘。
4. 实现简易的 LRU 机制，更好地重用缓存空间。

## 3. EasyFUSE 源代码结构说明

### (1) EasyFUSE 底层设施代码

1. [/src/disk.c](../src/disk.c) 磁盘层代码，位图结构部分代码也在这里。
2. [/src/log.c](../src/log.c) 日志层代码。
3. [/src/inode_cache.c](../src/inode_cache.c) Inode层代码。
4. [/src/block_cache.c](../src/block_cache.c) 块缓存层代码。
  
### (2) 实现的 libfuse 接口代码

1. 每个接口对应的实现在 [/src/syscalls](../src/syscalls) 目录下，实现了init，create，link，unlink，mkdir，rmdir，open，read，write，stat，symlink，readlink，readaddr 等等二十个左右必要和常用的接口。
2. 总体复用公共代码在 [/src/fs.c](../src/fs.c)。

### (3) 其他代码

1. [src/init_disk.c](../src/init_disk.c) 用于格式化磁盘，会清空磁盘所有数据，写入一些必要初始化磁盘数据，只需按照 EasyFUSE 时调用一次。
2. [src/util.c](../src/util.c) 用于输出各种辅助信息，以便debug的辅助函数库。
3. [src/main.c](../src/main.c) 主函数代码。
