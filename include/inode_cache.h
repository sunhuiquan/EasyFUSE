/* Inode缓存层(block layer)代码，提供磁盘上的Inode结构加载到内存的缓存机制，不过实际上Inode读写
 * 请求的操作是通过 block layer 这一中间层实现的，会先读到 block cache，然后再从 block cache
 * 读到 inode cache。
 */
#ifndef INODE_CACHE_H
#define INODE_CACHE_H

#include "inode.h"
#include <pthread.h>

extern struct inode_cache icache;

#define CACHE_INODE_NUM 50

struct inode_cache
{
	pthread_mutex_t cache_lock;			  // pthread互斥锁对整个 inode 缓存结构加锁
	struct inode inodes[CACHE_INODE_NUM]; // 缓存在内存中的真实存放位置，是一个全局变量
};

/* 初始化 inode cache 缓存，其实就是初始化它的互斥锁 */
int inode_cache_init();

/* 取出对应inum的inode内存结构，命中则直接取出，没命中则分配好缓存后去除(不过磁盘inode内容并未加载)，
 * 返回的inode指针是未加锁的。
 *
 * 另外第一次(缓存没命中的那次)valid为0，但不用担心，当我们使用这个inode指针的时候，就需要加锁，
 * 加锁内部会判断valid，为0则加载磁盘内容到缓存里面
 */
struct inode *iget(uint inum);

// 降低引用计数，并如果硬链接数和引用计数都为0时，释放对应磁盘
int inode_reduce_ref(struct inode *pi);

/* 释放inode所指向的所有数据块，注意调用这个的时候一定要持有pi的锁。 */
int inode_free_address(struct inode *pi);

/* 把dinode结构写到对应磁盘，注意这个是inode本身，而之前的wrietinode写的
 * 是指向的数据块中的数据，注意调用这个的时候一定要持有pi的锁。
 */
int inode_update(struct inode *pi);

/* 释放inode的锁并减引用 */
int inode_unlock_then_reduce_ref(struct inode *pi);

/* 在磁盘中找到一个未被使用的disk_inode结构，然后加载入内容并返回,通过iget返回，未持有锁且引用计数加一 */
struct inode *inode_allocate(ushort type);

/* 如果未加载到内存，那么将磁盘上的 dinode 加载到内存 icache 缓存中，这个加载的功能集成到ilock里面了，
 * 因为如果要使用，肯定是要先上锁，上锁的同时顺便就检测加载了
 */
static int inode_load(struct inode *pi);

/* 对内存中的inode对象加锁，然后如果未加载到内存，那么将磁盘上的内容加载到内存中，
 * 把 valid 等于0的情况判断是否加载到内存集成到加锁里面，是因为如果要操作inode的情况，
 * 都要加锁保证原子性，这样顺便检测是否已加载入就可以了
 */
int inode_lock(struct inode *pi);

/* 对内存中的inode对象解锁*/
int inode_unlock(struct inode *pi);

// 读 inode 里面的数据，实际上是通过 inode 和对应偏移量得到对应数据块的位置，然后读数据块
int readinode(struct inode *pi, void *dst, uint off, uint n);

// 写 inode 里面的数据，实际上是通过 inode 和对应偏移量得到对应数据块的位置，然后写数据块
int writeinode(struct inode *pi, void *src, uint off, uint n);

#endif