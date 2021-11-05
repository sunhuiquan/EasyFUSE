/* 实现简单的日志机制，保证数据一致性，遇到软件/物理错误能够恢复正确 */

#include "disk.h"
#include "fs.h"
#include "log.h"
#include "block_cache.h"

#include <string.h>
#include <pthread.h>

static int recover_from_log_disk();
static int write_to_data_disk(int is_recover);
static int disk_read_log_head();
static int write_log_head_to_disk();
static int copy_to_log_disk();

/**
 * 原理？？??
 */
struct log_head
{
	int ncopy;					 // 如果非0代表事务已提交，值代表已把数据从bcache拷贝到日志块的块数
								 // 注意这个ncopy是非头节点的日志块，所以最多是LOG_BLOCK_NUM - 1个
	int logs[LOG_BLOCK_NUM - 1]; // 每个日志块要写入的数据块逻辑块号，-1代表这全都是普通日志块
};

struct log
{
	// struct spinlock lock; 自旋锁？？??
	int syscall_num;		  // 当前正在写入日志块的写系统调用数量
	int is_committing;		  // 是否正在提交（commit）
	pthread_mutex_t log_lock; // 互斥锁
	struct log_head head;	  // 日志层的头节点块
};
struct log log;

/* 初始化日志层，这是内核加载文件系统，文件系统初始化的一部分，是开机启动操作系统的一部分，
 * 你要考虑上次关机是否正常，比如因断电关机，所以检测磁盘上的logging layer进行日志恢复操作。
 */
int init_log()
{
	if (pthread_mutex_init(&log.log_lock, NULL) == -1)
		return -1;
	if (recover_from_log_disk() == -1)
		return -1;
}

static int recover_from_log_disk()
{
	if (disk_read_log_head() == -1)
		return -1;
	if (write_to_disk(1) == -1)
		return -1;
	log.head.ncopy = 0;					// 恢复事务未提交状态（提交数为0）
	if (write_log_head_to_disk() == -1) // ??
		return -1;
	return 0;
}

/* 对提交的事务进行实际写入磁盘的操作，写入实质上是把先前事务已拷贝到磁盘上日志块的数据，
 * 再拷贝到磁盘上对应的数据块中。
 *
 * 这里才是真正写入磁盘的地方，之前的icache是通过把数据传到bcache来写入磁盘，而bcache
 * 实际上是借助logging layer这个中间层，先拷贝到日志磁盘块，然后在这里才是真正写入数据
 * 到磁盘块的地方，inode layer和bcache layer实际上都会不直接写入磁盘。
 *
 * 调用这个函数有两种途径，一个是日志层初始化的时候调用，另一种是写系统调用导致的，这两个
 * 方式的操作有所不同，所以需要is_recover这个参数体现是从哪一种途径调用的。
 *
 * 需要保证log.head.ncopy的值不超过普通日志块的数量LOG_BLOCK_NUM - 1，避免溢出。
 */
static int write_to_data_disk(int is_recover)
{
	struct cache_block *log_bbuf, *data_bbuf;
	for (int i = 0; i < log.head.ncopy; ++i) // 提交的已copy到日志块的块数，即我们要实际写到磁盘块的块数
	{
		if ((log_bbuf = block_read(superblock.log_block_startno + i + 1)) == NULL) // +1 避开头结点
			return NULL;
		if ((data_bbuf = block_read(log.head.logs[i])) == NULL) // logs是去掉头结点日志块开始的
			return NULL;
		memmove(data_bbuf->data, log_bbuf->data, BLOCK_SIZE);
		if (disk_write(data_bbuf) == -1)
			return -1;

		if (is_recover == 0) // 因为 write_log_head() 那里为了避免后面缓冲块被回收而增加了引用
			if (block_unlock_then_reduce_ref(write_log_head) == -1)
				return -1;

		if (block_unlock_then_reduce_ref(data_bbuf) == -1)
			return -1;
		if (block_unlock_then_reduce_ref(log_bbuf) == -1)
			return -1;
	}
	return 0;
}

/* 从磁盘读取头日志块到内存 */
static int disk_read_log_head()
{
	struct cache_block *bbuf = block_read(superblock.log_block_startno);
	if (bbuf == NULL)
		return -1;
	memmove(&log.head, bbuf->data, sizeof(struct log_head));
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
	return 0;
}

/* 将内存中的头节点日志块写入磁盘 */
static int write_log_head_to_disk()
{
	struct cache_block *bbuf;
	if ((bbuf = block_read(superblock.log_block_startno)) == NULL)
		return -1;
	memmove(bbuf->data, &log.head, sizeof(struct log_head));
	if (disk_write(bbuf) == -1)
		return -1;
	if (block_unlock_then_reduce_ref(bbuf) == -1)
		return -1;
	return 0;
}

/* 根据头日志块记录的bcache缓存块与普通日志块的映射关系，把数据从bcache拷贝到磁盘上对应的普通日志块，
 * 之后提交的时候会通过调用write_to_disk再把磁盘上普通日志块的数据拷贝到对应的磁盘数据块。
 */
static int copy_to_log_disk()
{
}

/* 提供的实际写入磁盘的接口函数，把要写的数据块的块号放入log head数组，确立一个空闲的普通日志块与之对应，
 * 之后copy_to_log会根据这个映射关系，把bcache缓存块写到磁盘上的log日志块。
 */
int write_log_head(struct cache_block *bbuf)
{
	if (log.head.ncopy == superblock.log_block_num - 1) // 当前这个事务已慢，无多余的空闲日志块
		return -1;
	if (log.syscall_num < 1) // 代表运行前没有began_log，不在事务内，当然如果不在事务内也不一定能通过这个发现
		return -1;

	if (pthread_mutex_lock(&log.log_lock) == -1)
		return -1;
	int i = 0;
	for (; i < log.head.ncopy; ++i)
		if (log.head.logs[i] == bbuf->blockno) // 吸收，不用做任何处理，因为此时没有实际提交写磁盘，所以下次写磁盘用的数据就是这个新的
		{
			if (pthread_mutex_unlock(&log.log_lock) == -1)
				return -1;
			return 0;
		}

	log.head.logs[i] = bbuf->blockno;
	++log.head.ncopy;
	if (block_increase_ref(bbuf) == -1) // 之后要用到这个缓存块，不让其被回收，提高效率（不然只会的block_read可能要重新加载）
		return -1;

	if (pthread_mutex_unlock(&log.log_lock) == -1)
		return -1;
	return 0;
}
