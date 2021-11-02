cmake_minimum_required(VERSION 3.5)

project(fuse_fs)

add_executable(init_disk src/init_disk.c src/disk.c src/util.c)
target_include_directories(init_disk PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(
  fuse_fs_main
  src/main.c
  src/block_cache.c
  src/disk.c
  src/fuse_fs_open.c
  src/inode_cache.c
  src/main.c
  src/util.c)
target_include_directories(fuse_fs_main PRIVATE ${PROJECT_SOURCE_DIR}/include)
