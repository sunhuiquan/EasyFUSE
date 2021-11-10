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
static int commit();

// 使用条件变量，避免了不断轮询请求释放锁来检测条件，避免CPU时间浪费。
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct log_head
{
	int ncopy;					 // 如果非0代表事务已提交，值代表已把数据从bcache拷贝到日志块的块数
								 // 注意这个ncopy是非头节点的日志块，所以最多是LOG_BLOCK_NUM - 1个
	int logs[LOG_BLOCK_NUM - 1]; // 每个日志块要写入的数据块逻辑块号，-1代表这全都是普通日志块
};

struct log
{
	int syscall_num;		  // 当前正在写入日志块的写系统调用数量
	int is_committing;		  // 是否正在提交（commit）
	pthread_mutex_t log_lock; // 互斥锁
	struct log_head head;	  // 日志层的头节点块
};
static struct log log;

/* 初始化日志层，这是内核加载文件系统，文件系统初始化的一部分，是开机启动操作系统的一部分，
 * 你要考虑上次关机是否正常，比如因断电关机，所以检测磁盘上的logging layer进行日志恢复操作。
 */
int init_log()
{
	if (pthread_mutex_init(&log.log_lock, NULL) == -1)
		return -1;
	if (recover_from_log_disk() == -1)
		return -1;
	return 0;
}

static int recover_from_log_disk()
{
	if (disk_read_log_head() == -1)
		return -1;
	if (write_to_data_disk(1) == -1)
		return -1;
	log.head.ncopy = 0;					// 恢复事务未提交状态（提交数为0）
	if (write_log_head_to_disk() == -1) // ??原子性考虑
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
 * write_log_head()保证log.head.ncopy的值不超过普通日志块的数量LOG_BLOCK_NUM - 1，避免溢出。
 */
static int write_to_data_disk(int is_recover)
{
	struct cache_block *log_bbuf, *data_bbuf;
	for (int i = 0; i < log.head.ncopy; ++i) // 提交的已copy到日志块的块数，即我们要实际写到磁盘块的块数
	{
		if ((log_bbuf = block_read(superblock.log_block_startno + i + 1)) == NULL) // +1 避开头结点
			return -1;
		if ((data_bbuf = block_read(log.head.logs[i])) == NULL) // logs是去掉头结点日志块开始的
			return -1;
		memmove(data_bbuf->data, log_bbuf->data, BLOCK_SIZE);
		if (disk_write(data_bbuf) == -1)
		{
			block_unlock_then_reduce_ref(data_bbuf);
			block_unlock_then_reduce_ref(log_bbuf);
			return -1;
		}

		if (is_recover == 0) // 因为 write_log_head() 那里为了避免后面缓冲块被回收而增加了引用
			if (block_reduce_ref(data_bbuf) == -1)
			{
				block_unlock_then_reduce_ref(data_bbuf);
				block_unlock_then_reduce_ref(log_bbuf);
				return -1;
			}

		if (block_unlock_then_reduce_ref(data_bbuf) == -1)
		{
			block_unlock_then_reduce_ref(log_bbuf);
			return -1;
		}
		if (block_unlock_then_reduce_ref(log_bbuf) == -1)
			return -1;
	}
	return 0;
}

/* 从磁盘读取头日志块到内存 */
static int disk_read_log_head()
{
	// 第一次安装读到的全是0，所以也是正确的
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
 * 之后提交的时候会通过调用write_to_data_disk再把磁盘上普通日志块的数据拷贝到对应的磁盘数据块。
 *
 * write_log_head()保证log.head.ncopy的值不超过普通日志块的数量LOG_BLOCK_NUM - 1，避免溢出。
 */
static int copy_to_log_disk()
{
	struct cache_block *log_bbuf, *data_bbuf;
	for (int i = 0; i < log.head.ncopy; ++i)
	{
		if ((data_bbuf = block_read(log.head.logs[i])) == NULL)
			return -1;
		if ((log_bbuf = block_read(superblock.log_block_startno + i + 1)) == NULL)
			return -1;

		memmove(log_bbuf->data, data_bbuf->data, BLOCK_SIZE);
		if (disk_write(log_bbuf) == -1)
		{
			block_unlock_then_reduce_ref(data_bbuf);
			block_unlock_then_reduce_ref(log_bbuf);
			return -1;
		}

		if (block_unlock_then_reduce_ref(log_bbuf) == -1)
		{
			block_unlock_then_reduce_ref(data_bbuf);
			return -1;
		}
		if (block_unlock_then_reduce_ref(data_bbuf) == -1)
			return -1;
	}
	return 0;
}

/* 提供的实际写入磁盘的接口函数，把要写的数据块的块号放入log head数组，确立一个空闲的普通日志块与之对应，
 * 之后copy_to_log会根据这个映射关系，把bcache缓存块写到磁盘上的log日志块。
 *
 * 注意这个写磁盘的接口并不是调用了就立即写，而是当out_transaction离开事务且事务内部的写系统调用数为0，
 * 调用commit提交，这个时候才真正地写磁盘操作，这个批处理，将多次写磁盘变成只写一次，提高效率。
 */
int write_log_head(struct cache_block *bbuf)
{
	// 当前这个事务已满，无多余的空闲日志块，保证log.head.ncopy的值不超过普通日志块的数量LOG_BLOCK_NUM - 1
	if (log.head.ncopy == superblock.log_block_num - 1)
		return -1;
	if (log.syscall_num < 1) // 代表运行前没有began_log，不在事务内，不过如果不在事务内不一定能通过这个发现
		return -1;

	if (pthread_mutex_lock(&log.log_lock) == -1)
		return -1;
	int i = 0;
	for (; i < log.head.ncopy; ++i)
		if (log.head.logs[i] == bbuf->blockno)
		{
			// 吸收，不用做任何处理，因为此时没有实际提交写磁盘（因为获得了锁，说明还没提交），
			// 所以下次提交写磁盘用的数据直接就是这个新的缓存块，我们不用作任何操作
			if (pthread_mutex_unlock(&log.log_lock) == -1)
				return -1;
			return 0;
		}

	log.head.logs[i] = bbuf->blockno;
	++log.head.ncopy;

	/* 这里这个地方非常关键，write_log_head调用完成后一般就会不再使用那个cache_block指针，因为它视角里是已经写入磁盘了，
	 * 所以很有可能会降低引用，所以这里increase ref避免write_log_head调用完成后ref到0被重用，导致之后使用一个其他的新来
	 * 的缓存块导致错误，这个ref增加直到disk_write完成，写入到磁盘，这时降低引用，因为此时磁盘内容就是最新的。
	 */
	if (block_increase_ref(bbuf) == -1)
		return -1;

	if (pthread_mutex_unlock(&log.log_lock) == -1)
		return -1;
	return 0;
}

/* 事务提交，先通过头结点中存储的对应关系，把缓存块写到磁盘上的日志块，然后更新磁盘上的头日志块，然后把磁盘上
 * 的日志块数据拷贝写到磁盘上的对应磁盘块，最后事务完成，ncopy变为0，重新重新磁盘上的头日志块信息。
 */
static int commit()
{
	if (log.head.ncopy > 0)
	{
		if (copy_to_log_disk() == -1)
			return -1;
		if (write_log_head_to_disk() == -1) // 原子性考虑??？？
			return -1;
		if (write_to_data_disk(0) == -1)
			return -1;
		log.head.ncopy = 0; // 原子性考虑??？？
		if (write_log_head_to_disk() == -1)
			return -1;
		return 0;
	}
	return -1; // 正常调用顺序不可能出现无事务提交的情况
}

/* log.head.ncopy是实际已经使用的普通日志块数，(log.syscall_num + 1)是已进入的系统调用数
 * (但还没退出事务转换成ncopy)加上即将进事务的自己，-1是去掉头结点日志块。
 *
 * in_transaction()进入事务，log.syscall_num会+1，这时候还没有实际执行写系统调用，所以不知道真实写使用
 * 的日志块数，所以我们使用MAX_WRITE_SYSCALLS这个最大的作为猜想值，这显然会导致过度估计对日志块的使用量
 * (除了write、unlink其他的写相关的系统调用一般用不到多少）；
 * 然后具体地执行写系统调用（内部是通过调用一次或多次write_log_head()，把要写的数据块的块号放入log head
 * 数组，确立一或多个空闲普通日志块与之对应（一次调用确立一个），然后commit的时候会经过copy_to_log_disk()
 * 和write_to_data_disk(0)来实际写入磁盘，并这个过程中实际提升了ncopy的值；
 * 最后out_transaction()退出事务，syscall_num会-1，因为之前无法确定的写使用日志块数已经增加到ncopy了，
 * 中间有个过程是syscall_num没有-1而且ncopy增加的过程，这里会过度增加认为的使用日志块数，不过并不影响，
 * 之后退出事务就会syscall_num -1变成真实大小。
 */

/* 进入一个事务（不是事务开始，因为这个日志为了效率采用批处理的方式，多个进程的写系统调用可以在同一个进程中）。
 * 这样一系列的begin_op结果会导致多个系统调用放到一个事务里面，通过批处理多个写系统调用只需要一次写磁盘。
 *
 * 需要确保一次操作不会写的块数超过MAX_WRITE_SYSCALLS。
 *
 * 使用条件变量，避免了不断轮询请求释放锁来检测条件，避免CPU时间浪费，当out_transaction()调用改变条件后，
 * 通过唤醒那些阻塞的条件变量，告知他们条件改变，然后让唤醒的条件变量请求锁后再次循环判断条件，得到处理。
 *
 * 标准上说唤醒只会唤醒一个线程，但是实际上实现很可能由于多处理器，导致惊群效应，唤醒了多个线程，他们直接也
 * 有个竞争需要注意，很可能唤醒多个，第一个加到锁的利用完了那个资源，然后其他被唤醒的线程就白白被唤醒了，所
 * 以需要再次循环检测条件，让这部分白白被唤醒的线程重新进入阻塞等待的状态；另外判断的条件可能是有种，照样需
 * 要再次重新循环一次判断，知道自己是因为什么具体的条件改变而被唤醒的。“POSIX规范为了简化实现，允许
 * pthread_cond_signal在实现的时候可以唤醒不止一个线程。” ——APUE
 *
 * 注意适当的提高in和out事务的范围可以不用那么频繁地调用，而且可以增加批处理的性能，提高效率，所以我们设计
 * 把这个范围放到一整个写系统调用上，比如一个write系统调用在开始的时候调用一个in_transaction，要结束的时候
 * 才调用一个out_transaction，把范围放到系统调用级别，而不是更细的粒度，利用好批处理，让最少一个，支持多个
 * 系统调用才来一次提交写入磁盘，而不是疯狂地一个系统调用多次in、out可能导致多次提交写磁盘，提高效率。
 */
int in_transaction()
{
	if (pthread_mutex_lock(&log.log_lock) == -1)
		return -1;
	while (1)
	{
		if (log.is_committing)
		{
			if (pthread_cond_wait(&cond, &log.log_lock) != 0)
				return -1;
		}
		else if (log.head.ncopy + (log.syscall_num + 1) * MAX_WRITE_SYSCALLS > LOG_BLOCK_NUM - 1)
		{
			if (pthread_cond_wait(&cond, &log.log_lock) != 0)
				return -1;
		}
		else
		{
			log.syscall_num += 1;
			if (pthread_mutex_unlock(&log.log_lock) == -1)
				return -1;
			break;
		}
	}
}

/* 离开事务，如果离开时无系统调用处于事务中，则提交事务 */
int out_transaction()
{
	int do_commit = 0;

	if (pthread_mutex_lock(&log.log_lock) == -1)
		return -1;
	--log.syscall_num;

	// 只有syscall_num为0的时候才可能提交，而处于提交状态的时候不可能有进入事务的，所以不可能有在提交状态下退出的情况
	if (log.is_committing)
		return -1;

	if (log.syscall_num == 0)
	{
		do_commit = 1;
		log.is_committing = 1; // 此时syscall_num为0，进入提交状态
	}
	else
	{
		if (pthread_cond_signal(&cond) != 0) // 非提交直接离开，大小条件改变，唤醒条件变量
			return -1;
	}

	if (pthread_mutex_unlock(&log.log_lock) == -1)
		return -1;

	/* 使用do_commit本地变量作标志，不用is_committing，这是为了防止设置is_committing为1，
	 * 并在判断完且重置为0的情况之前切换到别的进程，导致竞争错误，另一种思路是直接不释放锁，
	 * 一直保持锁到判断完重置为0后再释放锁，但这会导致锁的粒度太大，导致并发性能低下。
	 */
	if (do_commit)
	{
		commit(); // 提交
		if (pthread_mutex_lock(&log.log_lock) == -1)
			return -1;
		log.is_committing = 0;
		if (pthread_cond_signal(&cond) != 0) // 提交完成，is_committing条件改为0，唤醒条件变量
			return -1;
		if (pthread_mutex_unlock(&log.log_lock) == -1)
			return -1;
	}
	return 0;
}
