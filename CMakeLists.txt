cmake_minimum_required(VERSION 3.5)

project(fuse_fs)

add_executable(init_disk src/init_disk.c src/disk.c src/block_cache.c
                         src/util.c)
target_include_directories(init_disk PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(
  create_test
  test/create.c
  src/disk.c
  src/inode_cache.c
  src/block_cache.c
  src/log.c
  src/fuse_fs_open.c
  src/util.c)
target_include_directories(create_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
