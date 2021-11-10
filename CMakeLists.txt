cmake_minimum_required(VERSION 3.5)

project(fuse_fs)

add_executable(init_disk src/init_disk.c src/disk.c src/block_cache.c
                         src/util.c src/log.c)
target_include_directories(init_disk PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(
  fuse_fs
  src/main.c
  src/disk.c
  src/inode_cache.c
  src/block_cache.c
  src/log.c
  src/userspace_fs_init.c
  src/userspace_fs_stat.c
  src/userspace_fs_open.c
  src/util.c
  src/fs.c)
target_include_directories(fuse_fs PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(fuse_fs PRIVATE _FILE_OFFSET_BITS=64)
target_link_libraries(fuse_fs PRIVATE fuse)
