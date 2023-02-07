# EasyFUSE

```txt
 ______                    ______  _    _   _____  ______  
|  ____|                  |  ____|| |  | | / ____||  ____|  
| |__    __ _  ___  _     | |__   | |  | || (___  | |__  
|  __|  / _` |/ __|| | | ||  __|  | |  | | \___ \ |  __|  
| |____| (_| |\__ \| |_| || |     | |__| | ____) || |____  
|______|\__,_||___/ \__, ||_|      \____/ |_____/ |______|  
                     __/ |  
                    |___/
```

## 项目介绍

EasyFUSE 是一个出于学习目的轻量级用户态文件系统(FUSE)，支持并发和日志恢复，仿照 UNIX 文件系统实现，可运行在支持 libfuse 的 Linux 系统上。

## 项目文档

1. [EasyFUSE 的系统架构](./doc/architecture.md)
2. [EasyFUSE API 文档 to do 自动生成](./README.md)
3. [项目实现的一些细琐内容](./doc/libfuse_detail.md)
4. [测试和性能分析](./doc/analysis.md)
5. [参考资料](./doc/reference.md)  

## 如何使用

1. 安装 libfuse2

    ```bash
    sudo apt-get install libfuse2 libfuse-dev # 注意安装的是比较老的 libfuse2
    ```

2. 自动安装挂载

    (1) 运行ezbuild.sh脚本(自动构建的磁盘大小10M)

    ```bash
    ./ezbuild.sh
    cd build
    ./fuse_run mountdir # mountdir是自动创建的一个空目录，用于挂载，该程序作为守护进程运行
    ```

3. 手动安装挂载

    (1) 除了使用上一个的自动构建的脚本外，还可以手动安装。首先创建build目录，camke和make构建项目

    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

    (2) 在build创建一个叫diskimg的文件，用于充当模拟磁盘，注意必须要在build下，且名字必须是diskimg。文件大小可以自由指定，但不能过小导致之后的磁盘初始化程序失败。
  
     ```bash
    dd if=/dev/zero of=diskimg bs=1024 count=10000 # 在build路径下，名字必须是diskimg
    ```

    (3) 运行init_disk初始化第一步创建的文件，模拟格式化磁盘这个步骤（只需在安装的时候运行一次，之后除非格式化否则不再运行）

    ```bash
    ./init_disk
    ```

    (4) 运行fuse_run程序，-d代表可选的调试选项，无-d则该进程会成为守护进程后台运行，有则会在前台显示调试信息，然后是要挂载的目录路径参数，我们这里使用的是手动新建的一个路径。

    ```bash
    mkdir mountdir
    ./fuse_run mountdir # 无-d选项则成为守护进程后台运行
    ```

4. 使用FUSE

    (1). 注意前面的步骤是第一次安装才需要执行，之后不需要再执行上述步骤，如果不是要格式化磁盘，请不要使用init_disk程序，该程序会格式化您的磁盘，你只需要每次系统开机后，执行fuse_run程序加载我们的文件系统即可。

    ```bash
    # 每次开机后手动加载FUSE
    ./fuse_run mount_dir
    ```

    (2) 查看日志
	``` bash
	service rsyslog start # 比如对于 Ubuntu
	tail -n num /var/log/syslog # Ubuntu rsyslog
	```

    (2) 挂载信息

    ```bash
    mount | grep mountdir
	sudo umount mountdir # 删除前可能需要取消挂载
    ```

---

## Future

[Timeline](./doc/timeline.md) EasyFUSE的详细时间线

1. 补充FUSE系统调用
    - [ ] 实现 FUSE truncate 系统调用
    - [ ] 实现 FUSE lseek 系统调用
    - [ ] 实现 FUSE utime 系统调用
    - [ ] ...

2. 编写测试用例：
    - [ ] 并发测试
    - [ ] 日志恢复和事务处理测试
    - [ ] 集成测试
    - [ ] 性能分析

3. 添加其他功能模块：
    - [ ] 添加文件的那三个时间属性
    - [ ] 添加权限检查机制
    - [ ] ...

4. 其他：
    - [ ] 修改错误返回格式，统一标准
    - [ ] 设置开机自动挂载
