# fuse-toy-fs

My OS curriculum design, using FUSE(libfuse in linux) to build my user-mode toy file system.

## Index

- [项目介绍](#项目介绍)
- [项目原理](#项目原理)
- [项目实现](#项目实现)
- [测试使用](#测试使用)
- [参考文献](#参考文献)
- [Timeline](#Timeline)

## 项目介绍

  实现一个操作系统，一直被说成CS/CE本科生的三大终极浪漫之一(编译器，图形学，操作系统)，这个显然是不对的，因为怎么可能没有分布式和数据库呢？哈哈，这些是开玩笑的，计算机的领域繁杂而且隔领域如隔山，不过已经可见了操作系统的重要性，这个OS课设也算是我本科生涯的浪漫之一了。  
  如何实现一个文件系统？很有意思的是，对于一个简单的文件系统demo，FS本身的实现难度一点也不大，关键问题是你怎么让你的文件系统被OS支持呢？这反而是难点。  
  我第一个想法是直接linux写出fs实现，但这涉及到linux内核上的修改，修改linux内核编写我们的支持VFS的fs，这显然对我来说是不可能做到的，如果我能做到我自然可以直接去微软和FLAG了；第二个想法是让linux内核支持我们的fs太难，那么我找个小操作系统内核支持不就行了吗，我写过好几个xv6 os的模块，但如果这样做显然太过划水了，因为那些lab虽然难，但仍然不免是代码填空而已；第三个想法是我自己实现一个os内核，然后顺便把fs写了不就行了，然后我估计了下时间，想了想再怎么肝也要个三五个月，还是作罢。  
  所以最后，我打算使用FUSE(Filesystem in Userspace)，它作为一种通信管道，让一个守护进程把内核对VFS的调用并从内核态发到用户态的用户进程，然后我们根据libfuse的API实现我们的用户态fs，然后把返回结果通过libfuse发回内核态，完成对抽象文件系统VFS的接口支持，用linux下的libfuse库，实现我的用户态玩具文件系统，并在linux上测试和使用。

## 项目原理

## 项目实现

## 测试使用

## 参考文献

[1] _XV6 book_ <https://pdos.csail.mit.edu/6.828/2020/xv6/book-riscv-rev1.pdf>  
[2] _MIT 6.S081_ <https://pdos.csail.mit.edu/6.828/2020/index.html>  
[3] _libfuse doc_  <http://libfuse.github.io/doxygen/>  
[3] _libfuse github_ <https://github.com/libfuse/libfuse>  
[4] _深入理解计算机系统 (CS:APP)_  
[5] _LINUX/UNIX系统编程手册 (TLPI)_  
[6] _操作系统概念 (OSC)_

## Timeline

- [x] Make a plan for this whole project.
- [ ] Read xv6 CH8.
- [ ] Finish a filesystem based on risc-v xv6 operating system.
- [ ] Get understand how to implement a toy filesystem.
- [ ] Read libfuse document and learn how to use.
- [ ] Write a filesystem using libfuse under linux. (Split this goal.)
- [ ] Tests, tests, tests and so on and so forth.
- [ ] Convert this README.md into a PPT to end this project.
