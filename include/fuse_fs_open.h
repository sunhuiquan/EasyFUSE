/* 实现 libfuse open API */

#ifndef FUSE_FS_OPEN_H
#define FUSE_FS_OPEN_H

#include <sys/types.h>

/* 创建一个type类型、path路径的文件，返回的inode指针是持有锁的 */
struct inode *create(char *path, ushort type);

// 返回路径文件所在的目录的 inode，通过iget，返回未加锁，且已增加了引用计数
struct inode *find_dir_inode(char *path, char *name);

/* 通过目录 inode 查找该目录下对应 name 的 inode 并返回，通过iget，返回未加锁，且已增加了引用计数。
 * (内存中调用iget就说明有新的对象指向了这个inode缓存，调用iget就会导致引用计数+1)
 */
struct inode *dir_find(struct inode *pdi, char *name);

// 将name和inum组合成一条dirent结构，写入pdi这个目标inode结构，注意pdi是持有锁的
int add_dirent_entry(struct inode *pdi, char *name, uint inum);

/* wraper */
int wraper(char *path, ushort type);

#endif
