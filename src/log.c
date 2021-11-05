/* 实现简单的日志机制，保证数据一致性，遇到软件/物理错误能够恢复正确 */

#include "disk.h"
#include "fs.h"
#include "log.h"
#include "block_cache.h"

#include <string.h>
#include <pthread.h>

static int recover_from_log_disk();
static int disk_read_log_head();

struct log_head
{
	int ncopy;				 // 已把数据从bcache拷贝到日志块的块数
	int logs[LOG_BLOCK_NUM]; // 每个日志块要写入的数据块逻辑块号
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
	// to do
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
