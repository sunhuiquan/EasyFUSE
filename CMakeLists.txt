cmake_minimum_required(VERSION 3.5)

project(fuse_fs)

add_executable(init_disk src/init_disk.c src/disk.c src/util.c)
target_include_directories(init_disk PRIVATE ${PROJECT_SOURCE_DIR}/include)

# add_executable(
#   fuse_fs_main
#   main.c
#   block_cache.c
#   disk.c
#   fuse_fs_open.c
#   fuse_fs_stat.c
#   inode_cache.c
#   inode.c
#   main.c
#   util.c)
# target_include_directories(fuse_fs_main PRIVATE ${PROJECT_SOURCE_DIR}/include)
