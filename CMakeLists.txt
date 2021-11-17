cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug) # 为了能够调试，相当于-g选项
# set(CMAKE_BUILD_TYPE Release)

project(myfuse)

add_executable(init_disk src/init_disk.c src/disk.c src/block_cache.c
                         src/util.c src/log.c)
target_include_directories(init_disk PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(
  fuse_run
  # 底层机制代码
  src/main.c
  src/disk.c
  src/inode_cache.c
  src/block_cache.c
  src/log.c
  # FUSE系统调用代码
  src/fs.c
  src/syscalls/userspace_fs_init.c
  src/syscalls/userspace_fs_stat.c
  src/syscalls/userspace_fs_readdir.c
  src/syscalls/userspace_fs_mkdir.c
  src/syscalls/userspace_fs_rmdir.c
  src/syscalls/userspace_fs_create.c
  src/syscalls/userspace_fs_unlink.c
  src/syscalls/userspace_fs_link.c
  src/syscalls/userspace_fs_symlink.c
  src/syscalls/userspace_fs_readlink.c
  src/syscalls/userspace_fs_open.c
  src/syscalls/userspace_fs_read.c
  src/syscalls/userspace_fs_write.c
  # 辅助代码
  src/util.c)
target_include_directories(fuse_run PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(fuse_run PRIVATE _FILE_OFFSET_BITS=64)
target_link_libraries(fuse_run PRIVATE fuse)
