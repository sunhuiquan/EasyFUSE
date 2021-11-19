# Timeline

1. 实现FUSE底层机制：
    - [x] 安装 libfuse 库，熟悉 libfuse 接口的使用
    - [x] 通过文件模拟磁盘，设计磁盘块数据结构，磁盘层(相当于磁盘驱动代码)
    - [x] 初步实现块位图机制，通过块位图查找空闲的数据块(不包含Inode结构)
    - [x] 初步实现数据块缓冲机制
    - [x] 初步实现Inode块(指一个Inode结构算一个Inode块)缓冲机制
    - [x] 实现Inode块、Inode块缓冲的加锁机制
    - [x] 实现数据块、数据块缓冲的加锁机制
    - [x] CMake编译项目
    - [x] 实现logging layer，日志机制

2. 实现基本文件系统调用：
    - [x] 实现 FUSE getattr 系统调用
    - [x] 实现 FUSE readdir 系统调用
    - [x] 实现 FUSE mkdir 系统调用
    - [x] 实现 FUSE rmdir 系统调用
    - [x] 实现 FUSE create 系统调用
    - [x] 实现 FUSE unlink 系统调用
    - [x] 实现 FUSE link 系统调用
    - [x] 实现 FUSE symlink 系统调用
    - [x] 实现 FUSE readlink 系统调用
    - [x] 实现 FUSE rename 系统调用
    - [x] 实现 FUSE open 系统调用
    - [x] 实现 FUSE read 系统调用
    - [x] 实现 FUSE write 系统调用
    - [ ] 实现 FUSE truncate 系统调用
    - [ ] 实现 FUSE lseek 系统调用
    - [ ] 实现 FUSE utime 系统调用

3. 编写测试用例：
    - [ ] 并发测试
    - [ ] 日志恢复机制测试
    - [ ] 性能分析
    - [ ] 集成测试

4. 添加其他功能模块：
    - [ ] 添加时间属性
    - [ ] 添加权限检查机制
    - [ ] 添加文件加锁机制
    - [ ] 可能要更改为low-level API来降低接口的抽象程度
