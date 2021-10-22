# FUSE-FS

这是一个支持并发、支持日志恢复机制的一个类 UNIX 文件系统实现的用户态的文件系统。

## Index

- [FUSE-FS](#fuse-fs)  
- [Index](#index)  
- [项目介绍](#项目介绍)  
- [项目实现](#项目实现)  
- [测试使用](#测试使用)  
- [参考文献](#参考文献)  
- [Timeline](#timeline)

## 项目介绍

玩具文件系统机制非常多，每年关于 FS 机制的 OS 课设也是非常多，但绝大部分都是玩具中的玩具。这个 FUSE-FS 的区别于那些糟糕玩具的地方有三个点，那就是用户态 FS、支持并发、支持日志恢复。

  1. **用户态 FS：**  
  设计 FS 的时候，为了显得不那么玩具，我想要通过 linux VFS 接口，让 linux 支持。但是直接在 linux 内核中实在太过困难，所以我选择了 FUSE 来实现一个用户态的文件系统.那么什么是 FUSE 呢？
  简而言之，它作为一种通信机制，把内核对 VFS 接口的使用从内核态转发到用户态的我们实现的用户文件系统机制，然后用户文件系统把结果再通过 libfuse 发回内核，来完成对抽象文件系统(VFS)的实现。

  2. **并发支持：**  
  to do 还在画饼中

  3. **日志恢复机制：**  
  to do 还在画饼中

## 项目实现

1. 注意我们要使用libfuse的low level API，libfuse的高层API提供了一层打开文件描述和路径名的映射，使得高层API始终使用路径名作为参数。  
这样比如write(fd,...)，fd指向打开描述，打开描述指向Inode号，如果是low level API，我们会传递给libfuse低级API接口的就是这个Inode号；但如果是传递给高层接口，那么内部的那层抽象会把打开描述指向的映射到路径名，使得传递的始终是路径名。  
另外要注意的是，文件描述符和打开文件描述是进程属性，这里和libfuse无关，linux的那些系统调用会处理文件描述符和打开描述的数据结构，而libfuse的接口是从这里再往后面走，直接与Inode关联(低级接口)，或是提供一层映射始终使用路径名(高级接口)。

2. 要明白Linux如果有多个文件系统，那么首先每个文件系统都有自己的根目录，它们内部的实现的路径的/都是代表这个文件系统自己本身的根目录。一个文件系统B挂载到另一个文件系统A上，从A的角度上看B的根目录，这个路径是一个例如/dir/dir/mount_B这样的；但具体使用B内部的路径的时候时，B的挂载路径在B自己眼里是/的，B看不到上一层A的，这个A文件系统看起来的/dir/dir/mount_B路径，在B文件系统的视角就是/，看不到上一次A的那些路径。  
同理，要明白libfuse库挂载到一个路径下(比如linux原来的一个ext4文件系统的一个路径下)，发起关于文件系统的调用，如果是ext4文件系统，那么以他自己的/为基准；如果是对fuse文件系统发起系统调用的时候，传递给接口的path参数的路径是fuse文件系统自己的视角，意思是说比如在ext4眼里挂载路径是一个/a/b/c，挂载目录是c，但在我们实现的fuse眼里，libfuse接口传递的参数就是/，这个/代表的就是挂载目录(ext4看起来是/a/b/c在libfuse接口的参数是/)。  
知道了这个，我们就可以简单的把我们fuse文件系统的路径看作从/开始，而不用关上一层别的文件系统的路径视角了。

## 测试使用

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
- [x] 通过文件模拟磁盘，设计磁盘块数据结构，完成 disk layer
- [ ] 实现块缓冲机制，完成 block cache layer
- [ ] 实现 inode layer 和 bmap layer
- [ ] 通过 libfuse 实现 FS 的接口，并测试
- [ ] 通过同步机制，增加对并发的支持
- [ ] 实现日志恢复机制，增加 logging layer
