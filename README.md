# FUSE-FS

这是一个支持并发、支持日志恢复机制的一个类 UNIX 文件系统实现的用户态的文件系统。

## Index

- [FUSE-FS](#fuse-fs)  
- [Index](#index)  
- [项目介绍](#项目介绍)  
- [项目文件](#项目文件)  
- [项目原理](#项目原理)  
- [如何使用](#如何使用)  
- [参考文献](#参考文献)  
- [Timeline](#timeline)

## 项目介绍

玩具文件系统机制非常多，每年关于 FS 机制的 OS 课设也是非常多，但绝大部分都是玩具中的玩具。这个 FUSE-FS 的区别于那些糟糕玩具的地方有四点，那就是用户态、分层设计、并发支持、日志机制。

  1. **用户态：**  
  设计 FS 的时候，为了显得不那么玩具，我想要通过 linux VFS 接口，让 linux 支持。但是直接在 linux 内核中实在太过困难，所以我选择了 FUSE 来实现一个用户态的文件系统.那么什么是 FUSE 呢？
  简而言之，它作为一种通信机制，把内核对 VFS 接口的使用从内核态转发到用户态的我们实现的用户文件系统机制，然后用户文件系统把结果再通过 libfuse 发回内核，来完成对抽象文件系统(VFS)的实现。

  2. **分层设计：**
     <!-- 1. disk layer
     2. bitmap layer
     3. block cache layer
     4. log layer
     5. inode cache layer
     6. dictory layer
     7. pathname layer
     8. file descriptor layer -->

  3. **并发支持：**  
     1. 通过pthread的mutex作为使用的锁机制，保证临界区的原子性访问，分别对于缓存区整体、每一个缓存区中的元素拥有一个锁，建立两层锁机制。例如第一层锁是对于inode cache整体有一个锁保证了一个 inode 在缓存只有一个副本(因为to do)，以及缓存 inode 的引用计数正确（to do）；第二层锁是对每个内存中的 inode 都有一个锁，保证了可以独占访问 inode 的相关字段，以及 inode 所对应的数据块（to do）。对于数据块整体 cache 结构和内存中的单个数据块结构同理，也是这样的二层锁机制保证。
     2. 另外我们通过两层引用计数完成了分别对内存中的缓存结构和磁盘中的具体块的释放复用。例如一个内存中的 inode 的引用计数如果大于 0，则会使系统将该 inode 保留在缓存中，而不会重用该缓存 buffer；而每个磁盘上的 inode 结构都包含一个 nlink 硬链接计数字段，即引用该 inode 结构的目录项的数量，当 inode 的硬链接计数为零时，才会释放磁盘上这一个 inode 结构，让它被复用。数据块则是只有一个内存中的引用计数字段保证数据块缓存结构中的释放复用，磁盘上的数据块只是单纯的数据。

  4. **日志机制：**  
     to do 还在画饼中

## 项目文件

- [init_disk.c](src/init_disk.c) 用于初始化磁盘
- [disk.c](src/disk.c) 磁盘驱动层(disk layer)代码，里面是读写磁盘的驱动程序（我们的这里是通过linux文件API的模拟），也是逻辑块号转物理块号被驱动程序使用的地方（不过我们的实现使用的逻辑块与物理块是一对一的关系）。
- [block_cache.c](src/block_cache.c) 数据块缓存层(block layer)代码，提供磁盘上的数据块加载到内存的缓存机制。
- [inode_cache.c](src/inode_cache.c) Inode缓存层(block layer)代码，提供磁盘上的Inode结构加载到内存的缓存机制，不过实际上Inode读写请求的操作是通过 block layer 这一中间层实现的，会先读到 block cache，然后再从 block cache读到 inode cache。
<!-- - [log.c](src/log.c) logging layer，日志层 to do  -->
- [util.c](src/util.c) 用于输出各种辅助信息，以便debug的辅助函数库
- [fuse_fs_open.c](src/fuse_fs_open.c) 实现 libfuse open API

## 如何使用

1. 首先创建一个文件，用于充当模拟磁盘，内容可以清空（后面初始化会清零），路径任意（后面通过参数指定路径），但注意这不可以是一个空文件，这个文件的大小需要在这里就定下来。

```bash
dd if=/dev/zero of=diskimg bs=1024 count=10000
// 创建一个拥有10000个BLOCK_SIZE（指我们FS的块大小1024B）的文件，约10M
```

## 项目原理

1. 我们将通过linux的文件读写API来模拟磁盘驱动的读写，另外我们为了简单起见，我们FS系统中使用的逻辑块与我们的磁盘驱动程序（也就是我们通过linux文件读写调用模拟的代码）使用的物理块是一对一的关系，这样就大大简化了我们的驱动代码处理逻辑块与物理块映射的逻辑。

1. libfuse的高层API提供了一层打开文件描述和路径名的映射，使得高层API始终使用路径名作为参数。  
这样比如write(fd,...)，fd指向打开描述，打开描述指向Inode号，如果是low level API，我们会传递给libfuse低级API接口的就是这个Inode号；但如果是传递给高层接口，那么内部的那层抽象会把打开描述指向的映射到路径名，使得传递的始终是路径名。  
另外要注意的是，文件描述符和打开文件描述是进程属性，这里和libfuse无关，linux的那些系统调用会处理文件描述符和打开描述的数据结构，而libfuse的接口是从这里再往后面走，直接与Inode关联(低级接口)，或是提供一层映射始终使用路径名(高级接口)。

1. 要明白Linux如果有多个文件系统，那么首先每个文件系统都有自己的根目录，它们内部的实现的路径的/都是代表这个文件系统自己本身的根目录。一个文件系统B挂载到另一个文件系统A上，从A的角度上看B的根目录，这个路径是一个例如/dir/dir/mount_B这样的；但具体使用B内部的路径的时候时，B的挂载路径在B自己眼里是/的，B看不到上一层A的，这个A文件系统看起来的/dir/dir/mount_B路径，在B文件系统的视角就是/，看不到上一次A的那些路径。  
同理，要明白libfuse库挂载到一个路径下(比如linux原来的一个ext4文件系统的一个路径下)，发起关于文件系统的调用，如果是ext4文件系统，那么以他自己的/为基准；如果是对fuse文件系统发起系统调用的时候，传递给接口的path参数的路径是fuse文件系统自己的视角，意思是说比如在ext4眼里挂载路径是一个/a/b/c，挂载目录是c，但在我们实现的fuse眼里，libfuse接口传递的参数就是/，这个/代表的就是挂载目录(ext4看起来是/a/b/c在libfuse接口的参数是/)。  
知道了这个，我们就可以简单的把我们fuse文件系统的路径看作从/开始，而不用关上一层别的文件系统的路径视角了。

1. 使用pthread库的时候，要注意下pthread库里面的函数返回值错误时不是-1，而是errno值，这个错误返回的风格和C标准库的风格不一样。

## 参考文献

[1] _XV6 book_ <https://pdos.csail.mit.edu/6.828/2020/xv6/book-riscv-rev1.pdf>  
[2] _MIT 6.S081_ <https://pdos.csail.mit.edu/6.828/2020/index.html>  
[3] _libfuse doc_ <http://libfuse.github.io/doxygen/>  
[3] _libfuse github_ <https://github.com/libfuse/libfuse>  
[4] _深入理解计算机系统 (CS:APP)_  
[5] _LINUX/UNIX 系统编程手册 (TLPI)_  
[6] _操作系统概念 (OSC)_

## Timeline

- [x] 安装 libfuse 库，熟悉 libfuse 接口的使用
- [x] 通过文件模拟磁盘，设计磁盘块数据结构，磁盘层(相当于磁盘驱动代码)
- [x] 初步实现块位图机制，通过块位图查找空闲的数据块(不包含Inode结构)
- [x] 初步实现数据块缓冲机制
- [x] 初步实现Inode块(指一个Inode结构算一个Inode块)缓冲机制
- [x] 初步实现Inode块、Inode块缓冲的加锁机制
- [x] 初步实现数据块、数据块缓冲的加锁机制
- [x] CMake编译项目
- [ ] 实现创建文件功能并测试
- [ ] ...
