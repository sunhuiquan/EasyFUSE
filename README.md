# FUSE-FS

这是一个支持并发、支持日志恢复机制的一个类 UNIX 文件系统实现的用户态的文件系统。

## Index

- [项目介绍](#项目介绍)
- [项目实现](#项目实现)
- [测试使用](#测试使用)
- [参考文献](#参考文献)
- [Timeline](#Timeline)

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

- [ ] 安装 libfuse 库，熟悉 libfuse 接口的使用，写 demo 测试
- [ ] 通过文件模拟磁盘，设计磁盘块数据结构，完成 disk layer
- [ ] 实现块缓冲机制，完成 block cache layer
- [ ] 实现 inode layer 和 bmap layer
- [ ] 通过 libfuse 实现 FS 的接口，并测试
- [ ] 通过同步机制，增加对并发的支持
- [ ] 实现日志恢复机制，增加 logging layer
