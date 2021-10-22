#include "global.h"
#include "utils.h"

#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
char *vdisk_path = "/home/hadoop/code/homework/newdisk";
char *root_path = "/";

void utils_init()
{

	FILE *fp = NULL;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
	{
		fprintf(stderr, "unsuccessful!\n");
		return;
	}
	//读超级块
	struct super_block *super_block_record = malloc(sizeof(struct super_block));
	fread(super_block_record, sizeof(struct super_block), 1, fp);
	//用超级块中的fs_size初始化全局变量
	TOTAL_BLOCK_NUM = super_block_record->fs_size;
	fclose(fp);
	fslog("INIT", "init success\n");
}

/**
 * The src_ attribute assignment to dest_
 * 读出src_的属性到dest_
 */
void read_file_dir(struct u_fs_file_directory *dest_,
				   struct u_fs_file_directory *src_)
{
	//读出属性
	strcpy(dest_->fname, src_->fname);
	strcpy(dest_->fext, src_->fext);
	dest_->fsize = src_->fsize;
	dest_->nStartBlock = src_->nStartBlock;
	dest_->flag = src_->flag;
}

/**
 * According to the block number move the pointer, read the assignment to blk_info
 * 根据块号移动指针，读出内容赋值给blk_info
 * return 0 on success , -1 on failed
 *
 * blk_pos : 将要读取的块的块号
 * block :   存放读到的块的内容
 */
int get_blkinfo_from_read_blkpos(long blk, struct u_fs_disk_block *blk_info)
{
	FILE *fp;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
		return -1;
	fseek(fp, blk * FS_BLOCK_SIZE, SEEK_SET);
	fread(blk_info, sizeof(struct u_fs_disk_block), 1, fp);
	if (ferror(fp) || feof(fp))
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

/**
 * Blk_info will be written into the specified block number of disk blocks
 * 将blk_info内容写进指定块号的磁盘块
 * return 0 on success , -1 on failed
 *
 * blk : 将要写入的块的块号
 * block :   将写入磁盘块的内容
 */
int write_blkinfo_start_blk(long blk, struct u_fs_disk_block *blk_info)
{
	FILE *fp;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
		return -1;
	if (fseek(fp, blk * FS_BLOCK_SIZE, SEEK_SET) != 0)
	{
		fclose(fp);
		return -1;
	}

	fwrite(blk_info, sizeof(struct u_fs_disk_block), 1, fp);
	if (ferror(fp) || feof(fp))
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

/**
 * Open a path of the file or directory, read the attribute assigned to return_dirent
 * 读取path对应dirent（文件或目录）信息，并赋值到return_dirent中
 * return 0 on success
 * return -1 on failed
 *
 * path: 欲获取文件信息的文件路径
 * return_dirent: 存放文件信息
 *
 */
int get_attr_from_open_pathblock(const char *path,
								 struct u_fs_file_directory *attr)
{

	// fslog(tag,"path is %s\n",path);
	struct u_fs_disk_block *blk_info;
	blk_info = malloc(sizeof(struct u_fs_disk_block));
	/* read the super block*/
	if (get_blkinfo_from_read_blkpos(0, blk_info) == -1)
	{
		fslog("readblk", "%s325path\n", path);
		free(blk_info);
		return -1;
	}

	struct super_block *sb_record;
	sb_record = (struct super_block *)blk_info;
	long start_blk;
	start_blk = sb_record->first_blk;

	char *p, *q, *tmp_path;
	tmp_path = strdup(path);
	p = tmp_path;
	// tmp_path=/
	if (strcmp(path, "/") == 0)
	{
		attr->flag = 2;
		attr->nStartBlock = start_blk;
		free(blk_info);
		return 0;
	}
	//处理路径
	if (!p)
	{
		free(blk_info);
		return -1;
	}
	p++;
	q = strchr(p, '/');
	if (q != NULL)
	{
		tmp_path++; // tmp_path为上级目录名
		*q = '\0';
		q++;
		p = q; // p为文件名
	}
	q = strchr(p, '.');
	if (q != NULL)
	{
		*q = '\0';
		q++; // q为文件名
	}

	//读出根目录块信息
	if (get_blkinfo_from_read_blkpos(start_blk, blk_info) == -1)
	{
		free(blk_info);
		return -1;
	}
	//读出根目录文件信息
	struct u_fs_file_directory *file_dir =
		(struct u_fs_file_directory *)blk_info->data;
	int offset = 0;
	// fslog("open","%s\n",tmp_path);

	if (*tmp_path != '/')
	{ //上级目录为根目录下目录
		while (offset < blk_info->size)
		{ //找目录
			if (strcmp(file_dir->fname, tmp_path) == 0 && file_dir->flag == 2)
			{									   //找到该目录
				start_blk = file_dir->nStartBlock; //进入该目录
				break;
			}
			//没找到目录继续找
			file_dir++;
			offset += sizeof(struct u_fs_file_directory);
		}
		// fslog("open385","%s\n",tmp_path);
		//最终还是没找到。。
		if (offset == blk_info->size)
		{
			// fslog("open388","%s\n",tmp_path);
			free(blk_info);
			return -1;
		}
		//读出上级目录的块信息
		if (get_blkinfo_from_read_blkpos(start_blk, blk_info) == -1)
		{
			free(blk_info);
			return -1;
		}
		file_dir = (struct u_fs_file_directory *)blk_info->data;
	}
	// fslog("open399","%s\n",tmp_path);
	offset = 0;
	while (offset < blk_info->size)
	{ //在文件/目录的上级目录中找文件/目录

		if (file_dir->flag != 0 && strcmp(file_dir->fname, p) == 0 && (q == NULL || strcmp(file_dir->fext, q) == 0))
		{ // find!
			//进入文件/目录所在块
			start_blk = file_dir->nStartBlock;
			//读出属性
			read_file_dir(attr, file_dir);
			free(blk_info);
			return 0;
		}
		//读下一个文件
		file_dir++;
		offset += sizeof(struct u_fs_file_directory);
	}
	// fslog("open415","%s\n",tmp_path);
	free(blk_info);
	return -1;
}

/**
 * Create a file or directory path is &path.
 * Flag 1 files, 2 directory
 * 创建path的文件或目录，flag 为1表示文件，2为目录
 *
 * return 0 on success , -1 on failed
 */
int create_file_dir(const char *path, int flag)
{
	int res;

	long p_dir_blk;
	char *p = malloc(15 * sizeof(char)), *q = malloc(15 * sizeof(char));
	// fslog("readblk","%s\n177",path);
	//拆分路径，找到父级目录块
	if ((res = dv_path(p, q, path, &p_dir_blk, flag)))
	{
		fslog("dmcreate", "%lld\n", res);
		freePtrs(p, q, NULL);
		return res;
	}
	fslog("create", "%s183\n", path);
	struct u_fs_disk_block *blk_info = malloc(sizeof(struct u_fs_disk_block));
	//从目录块中读取目录信息到blk_info
	if (get_blkinfo_from_read_blkpos(p_dir_blk, blk_info) == -1)
	{
		freePtrs(blk_info, p, q, NULL);
		return -ENOENT;
	}
	struct u_fs_file_directory *file_dir =
		(struct u_fs_file_directory *)blk_info->data;

	int offset = 0;
	int pos = blk_info->size;
	//遍历目录下的所有文件和目录，如果已存在同名文件或目录，返回
	if ((res = exist_check(file_dir, p, q, &offset, &pos, blk_info->size, flag)))
	{
		freePtrs(blk_info, p, q, NULL);
		return res;
	}
	file_dir += offset / sizeof(struct u_fs_file_directory);

	long *tmp = malloc(sizeof(long));
	if (pos == blk_info->size)
	{
		//块扩容
		if (blk_info->size > MAX_DATA_IN_BLOCK)
		{
			//当前块放不下目录内容
			if ((res = enlarge_blk(p_dir_blk, file_dir, blk_info, tmp, p, q,
								   flag)))
			{
				freePtrs(p, q, blk_info, NULL);
				return res;
			}
			freePtrs(p, q, blk_info, NULL);
			return 0;
		}
		else
		{
			//块容量足够，直接加size
			blk_info->size += sizeof(struct u_fs_file_directory);
			// fslog("free","succeed222\n");
		}
	}
	else
	{ // flag=0
		offset = 0;
		file_dir = (struct u_fs_file_directory *)blk_info->data;

		fslog("flag=0", "%d\t%d\t\n", offset, pos);
		// file_dir+=blk_info->size;
		while (offset < pos)
			file_dir++;
	}
	init_file_dir(file_dir, p, q, flag);
	//找到空闲块作为起始块
	tmp = malloc(sizeof(long));

	if ((res = get_free_blocks(1, tmp)) == 1)
	{
		file_dir->nStartBlock = *tmp;
	}
	else
	{
		// fslog("create","%d\n",rs);
		freePtrs(blk_info, p, q, NULL);
		return -errno;
	}
	free(tmp);

	//将要创建的文件或目录信息写入上层目录中
	write_blkinfo_start_blk(p_dir_blk, blk_info);
	new_emp_blk(blk_info);
	//文件起始块内容为空
	write_blkinfo_start_blk(file_dir->nStartBlock, blk_info);

	freePtrs(p, q, blk_info, NULL);

	return 0;
}

/**
 * Find a num continuous free block, get free block start block number start_block,
 * return to find the number of consecutive idle blocks (maximum otherwise found)
 * 找到num个连续空闲块，得到空闲块区的起始块号start_block，返回找到的连续空闲块个数（否则返回找到的最大值）
 *
 * num: 要寻找的连续空闲块个数
 * start_blk: 找到的空闲块的起始号码
 */
int get_free_blocks(int num, long *start_blk)
{
	// bitmap中super block和bitmap block区和rootdir
	//从头开始找
	*start_blk = 1 + BITMAP_BLOCK + 1;
	int tmp = 0;
	FILE *fp = NULL;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
		return 0;
	int start, left;
	unsigned int mask, f;
	int *flag;

	int max = 0;
	long max_start = -1;

	while (*start_blk < TOTAL_BLOCK_NUM - 1)
	{
		// bitmap中一个byte，描述8个block（*start_blk）的使用情况，要处理8个以内的block时，需要位操作
		start = *start_blk / 8;
		left = *start_blk % 8;
		mask = 1;
		mask <<= left;
		fseek(fp, FS_BLOCK_SIZE + start, SEEK_SET); //跳过super block，跳到bitmap中start_blk指定位置
		flag = malloc(sizeof(int));
		fread(flag, sizeof(int), 1, fp); //读出32个bit
		f = *flag;
		for (tmp = 0; tmp < num; tmp++)
		{
			if ((f & mask) == mask) // mask为1的位，f中为1，该位被占用，为找连续块，跳出
				break;
			if ((mask & 0x80000000) == 0x80000000)
			{ // mask最高位为1，已移位到最高位，32个bit已读完
				//读下32bit
				fread(flag, sizeof(int), 1, fp);
				f = *flag;
				mask = 1; //指向32个bit的最低位
			}
			else
				//位为1,左移1位，检查下一位是否可用
				mask <<= 1;
		}
		// tmp为找到的可用连续块数目
		if (tmp > max)
		{
			//记录这个连续块的起始位
			max_start = *start_blk;
			max = tmp;
		}
		//如果后来找到的连续块数目少于之前找到的，不做替换
		//找到了num个连续的空白块
		if (tmp == num)
			break;
		//只找到了tmp个可用block，小于num，要重新找，如果能找到大于上一次找到的空闲块数，更新起始块号
		*start_blk = (tmp + 1) + *start_blk;
		tmp = 0;
		//找不到空闲块
		// fslog("free","succeed664");
		// if(max==0)return -1;
	}
	*start_blk = max_start;
	fclose(fp);
	int j = max_start;
	int i;
	for (i = 0; i < max; ++i)
	{
		//找到的块标为已使用
		if (utils_set_blk(j++, 1) == -1)
		{
			free(flag);
			return -1;
		}
	}
	free(flag);
	return max;
}

/**
 * Remove a file or directory path is &path.
 * Flag 1 files, 2 directory
 * 删除path的文件或目录，flag 为1表示文件，2为目录
 *
 * return 0 on success , -1 on failed
 */
int rm_file_dir(const char *path, int flag)
{
	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));
	//读取文件属性
	if (get_attr_from_open_pathblock(path, attr) == -1)
	{
		free(attr);
		return -ENOENT;
	}
	// flag与指定的不一致
	if (flag == 1 && attr->flag == 2)
	{
		free(attr);
		return -EISDIR;
	}
	else if (flag == 2 && attr->flag == 1)
	{
		free(attr);
		return -ENOTDIR;
	}
	//从起始块清空后续块
	struct u_fs_disk_block *blk_info = malloc(sizeof(struct u_fs_disk_block));
	if (flag == 1)
	{
		long next_blk = attr->nStartBlock;
		nextClear(next_blk, blk_info);
	}
	else if (!is_empty(path))
	{ //目录非空
		freePtrs(blk_info, attr, NULL);
		return -ENOTEMPTY;
	}

	attr->flag = 0; //设置为未使用
	if (utils_setattr(path, attr) == -1)
	{
		freePtrs(blk_info, attr, NULL);
		return -1;
	}

	freePtrs(blk_info, attr, NULL);
	return 0;
}

/**
 * This function should be able to output the running status information into log_path file
 * in order to debug or view.
 *
 *  tag: 标志符
 *  format：字符串格式
 */
void fslog(const char *tag, const char *format, ...)
{
	/*if (!logFlag)
		return;
	char* logPath = strdup(log_path);
	char* q = logPath;
	q += strlen(vdisk_path) - 7;
	*q = 'l';
	q++;
	*q = 'o';
	q++;
	*q = 'g';
	q++;
	*q = '\0';
	FILE* fp = NULL;
	fp = fopen(log_path, "at");
	if (fp == NULL) {
		return;
	}
	time_t now;         //实例化time_t结构
	struct tm *timenow;         //实例化tm结构指针
	time(&now);
	//time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
	timenow = localtime(&now);
	//localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
	fprintf(fp, "%d-%d-%d %d:%d:%d\t", timenow->tm_year + 1900,
			timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour,
			timenow->tm_min, timenow->tm_sec);
	//上句中asctime函数把时间转换成字符，通过printf()函数输出
	fprintf(fp, "%s:\t", tag);
	va_list ap;
	va_start(ap, format);
	vfprintf(fp, format, ap);
	va_end(ap);
	free(logPath);
	fclose(fp);*/
}
/**
 * This function should be release one or more pointers.
 *
 * ptr: The pointer to be released
 */
void freePtrs(void *ptr, ...)
{
	va_list argp;  /* 定义保存函数参数的结构 */
	int argno = 0; /* 纪录参数个数 */
	char *para;	   /* 存放取出的字符串参数 */
	va_start(argp, ptr);
	free(ptr);
	while (1)
	{
		para = va_arg(argp, void *); /*    取出当前的参数，类型为char *. */
		if (para == NULL)
			break;
		free(para);
		argno++;
	}
	va_end(argp); /* 将argp置为NULL */
}

/**
 * 通过路径path获得文件的信息，赋值给stat stbuf
 * return 0 mean success
 * param
 * path：文件路径
 * stbuf：stat文件信息
 */
int utils_getattr(const char *path, struct stat *stbuf)
{
	fslog("utils_getattr", "path :%s", path);

	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));
	//非根目录
	if (get_attr_from_open_pathblock(path, attr) == -1)
	{
		free(attr);
		return -ENOENT;
	}
	memset(stbuf, 0, sizeof(struct stat));
	// All files will be full access (i.e., chmod 0666), with permissions to be mainly ignored
	if (attr->flag == 2)
	{ // directory
		stbuf->st_mode = S_IFDIR | 0666;
	}
	else if (attr->flag == 1)
	{ // regular file
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_size = attr->fsize;
	}
	free(attr);
	return 0;
}
/**
 * This function should look up the input path.
 * ensuring that it's a directory, and then list the contents.
 * return 0 on success
 */
int utils_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				  off_t offset)
{

	struct u_fs_disk_block *blk_info;
	struct u_fs_file_directory *attr;
	blk_info = malloc(sizeof(struct u_fs_disk_block));
	attr = malloc(sizeof(struct u_fs_file_directory));

	if (get_attr_from_open_pathblock(path, attr) == -1)
	{ //打开path指定的文件，将文件属性读到attr中
		freePtrs(attr, blk_info, NULL);
		return -ENOENT;
	}

	long start_blk = attr->nStartBlock;
	if (attr->flag == 1)
	{ // regual file
		freePtrs(attr, blk_info, NULL);
		return -ENOENT;
	}
	if (get_blkinfo_from_read_blkpos(start_blk, blk_info))
	{ // assign blk_info
		freePtrs(attr, blk_info, NULL);
		return -ENOENT;
	}

	// all directory had . and .. child_directory
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	//顺序查找,并向buf增添文件名
	struct u_fs_file_directory *file_dir =
		(struct u_fs_file_directory *)blk_info->data;
	int pos = 0;
	char name[MAX_FILENAME + MAX_EXTENSION + 2];
	while (pos < blk_info->size)
	{
		strcpy(name, file_dir->fname);
		if (strlen(file_dir->fext) != 0)
		{
			strcat(name, ".");
			strcat(name, file_dir->fext);
		}
		if (file_dir->flag != 0 && name[strlen(name) - 1] != '~' && filler(buf, name, NULL, 0)) // add filename into buf
			break;
		file_dir++;
		pos += sizeof(struct u_fs_file_directory);
	}

	freePtrs(attr, blk_info, NULL);
	return 0;
}

/**
 * Write size bytes from buf into file starting from offset
 * 将buf里大小为size的内容，写入path指定的起始块后的第offset
 * （就像从指定行的第几个字符开始写的意思）
 */
int utils_write(const char *path, const char *buf, size_t size, off_t offset)
{
	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));
	//打开path指定的文件，将文件属性读到attr中
	get_attr_from_open_pathblock(path, attr);
	fslog("write", "%s:%d\n", path, attr->fsize);
	if (offset > attr->fsize)
	{
		free(attr);
		return -EFBIG;
	}

	long start_blk = attr->nStartBlock;
	if (start_blk == -1)
	{
		free(attr);
		return -errno;
	}

	int res;
	struct u_fs_disk_block *blk_info = malloc(sizeof(struct u_fs_disk_block));
	int para_offset = offset;

	int num;
	// fslog("write","blk_info:start_blk:%d\n",blk_info->nNextBlock);
	//找到offset指定的块位置
	if ((res = (find_off_blk(&start_blk, &offset, blk_info))))
	{
		return res;
	}
	// fslog("write","blk_info:start_blk:%d\n",blk_info->nNextBlock);
	//找到offset所在块
	char *pt = blk_info->data;
	//找到offset所在块中offset位置
	pt += offset;

	int towrite = 0;
	int writen = 0;
	//磁盘块剩余空间小于文件大小，则写满磁盘块剩余空间，否则写入整个文件
	towrite = (MAX_DATA_IN_BLOCK - offset < size ? MAX_DATA_IN_BLOCK - offset : size);
	strncpy(pt, buf, towrite); //写入长度为towrite的内容
	buf += towrite;			   //移到待写处
	blk_info->size += towrite; //已写增加towrite
	writen += towrite;		   //已写
	size -= towrite;		   //待写

	//构造一个block作为待写入文件的新块
	long *next_blk = malloc(sizeof(long));
	if (size > 0)
	{
		//还有数据未写入
		//找到num个连续的空闲块写入
		num = get_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk);
		// num可能小于size/MAX_DATA_IN_BLOCK+1，先写入一部分
		if (num == -1)
		{
			freePtrs(attr, blk_info, next_blk, NULL);
			return -errno;
		}
		blk_info->nNextBlock = *next_blk;
		// start_blk更新为扩大了的块
		write_blkinfo_start_blk(start_blk, blk_info);
		int i;
		while (1)
		{
			for (i = 0; i < num; ++i)
			{
				//在新块写入size大小的数据，逻辑同前
				towrite = (MAX_DATA_IN_BLOCK < size ? MAX_DATA_IN_BLOCK : size);
				blk_info->size = towrite;
				strncpy(blk_info->data, buf, towrite);
				buf += towrite;
				size -= towrite;
				writen += towrite;

				if (size == 0) //已写完则无后续块
					blk_info->nNextBlock = -1;
				else
					//未写完增加后续块
					blk_info->nNextBlock = *next_blk + 1;
				//更新块为扩容后的块
				write_blkinfo_start_blk(*next_blk, blk_info);
				*next_blk = *next_blk + 1;
			}
			if (size == 0) //已写完
				break;
			// num小于size/504+1,找到的num不够，继续找
			num = get_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk);
			if (num == -1)
			{
				freePtrs(attr, blk_info, next_blk, NULL);
				return -errno;
			}
		}
	}
	else if (size == 0)
	{
		//块空间小于剩余空间
		//缓存nextblock
		long next_blk;
		next_blk = blk_info->nNextBlock;
		//终点块
		blk_info->nNextBlock = -1;
		write_blkinfo_start_blk(start_blk, blk_info);
		//清理nextblock（原nextblock链不再需要）
		nextClear(next_blk, blk_info);
	}
	size = writen;

	//修改被写文件大小信息为写入起始位置+写入内容大小
	attr->fsize = para_offset + size;
	if (utils_setattr(path, attr) == -1)
	{
		size = -errno;
	}
	fslog("writesucceed", "%s:%d\n", path, size);
	freePtrs(attr, blk_info, next_blk, NULL);
	return size;
}

/**
 * 根据路径path找到文件起始位置，再偏移offset长度开始读取数据到buf中，返回文件大小
 * return size
 * param
 * path：路径
 * buf：存储path对应文件信息
 * size：文件大小
 * offset：读取时候的偏移量
 * fi：fuse的文件信息
 */
int utils_read(const char *path, char *buf, size_t size, off_t offset)
{

	// the file attribute(the path point to )
	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));

	// get the file's attribute with path
	if (get_attr_from_open_pathblock(path, attr) == -1)
	{
		free(attr);
		return -ENOENT;
	}
	// directory can't be read ,it must be a file
	if (attr->flag == 2)
	{
		free(attr);
		return -EISDIR;
	}

	struct u_fs_disk_block *blk_info; // the file content's block
	blk_info = malloc(sizeof(struct u_fs_disk_block));

	//根据文件信息读取文件内容
	if (get_blkinfo_from_read_blkpos(attr->nStartBlock, blk_info) == -1)
	{
		freePtrs(attr, blk_info, NULL);
		return -1;
	}

	/*顺序查找文件内容块,读取并添入buf中*/

	//要读取的位置在文件范围内
	if (offset < attr->fsize)
	{
		if (offset + size > attr->fsize)
			size = attr->fsize - offset;
	}
	else
		size = 0;
	/*int tmp = size;
	 char *pt = blk_info->data;
	 pt += offset;
	 strcpy(buf, pt);
	 tmp -= blk_info->size;
	 while (tmp > 0) {
	 if (get_blkinfo_from_read_blkpos(blk_info->nNextBlock, blk_info) == -1)
	 break;
	 strcat(buf, blk_info->data);
	 tmp -= blk_info->size;
	 }
	 freePtrs(attr, blk_info, NULL);
	 return size;*/

	int real_offset, blk_num, i, ret = 0;
	long start_blk = blk_info->nNextBlock;

	blk_num = offset / MAX_DATA_IN_BLOCK;
	real_offset = offset % MAX_DATA_IN_BLOCK;

	//跳过offset超过的block块,找到offset所指的开始位置的块
	for (i = 0; i < blk_num; i++)
	{
		if (get_blkinfo_from_read_blkpos(blk_info->nNextBlock, blk_info) == -1 || start_blk == -1)
		{
			printf("read_block_from_pos failed!\n");
			freePtrs(attr, blk_info, NULL);
			return -1;
		}
	}
	//文件内容可能跨多个block块
	int temp = size;
	char *pt = blk_info->data;
	pt += real_offset;
	ret = (MAX_DATA_IN_BLOCK - real_offset < size ? MAX_DATA_IN_BLOCK - real_offset : size);
	memcpy(buf, pt, ret);
	temp -= ret;

	while (temp > 0)
	{
		if (get_blkinfo_from_read_blkpos(blk_info->nNextBlock, blk_info) == -1)
		{
			printf("read_block_from_pos failed!\n");
			break;
		}
		if (temp > MAX_DATA_IN_BLOCK)
		{
			memcpy(buf + size - temp, blk_info->data, MAX_DATA_IN_BLOCK);
			temp -= MAX_DATA_IN_BLOCK;
		}
		else
		{
			memcpy(buf + size - temp, blk_info->data, temp);
			break;
		}
	}
	freePtrs(attr, blk_info, NULL);
	return size;
}

/**
 * This function should not be modified,
 * as you get the full path every time any of the other functions are called.
 * return 0 on success
 * 按路径打开文件/目录
 * return 0 mean success
 */

int utils_mkdir(const char *path, mode_t mode)
{
	int res = create_file_dir(path, 2);
	return res;
}

int utils_rmdir(const char *path)
{
	int res = rm_file_dir(path, 2);
	return res;
}

int utils_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res = create_file_dir(path, 1);
	return res;
}

int utils_unlink(const char *path)
{
	int res = rm_file_dir(path, 1);
	return res;
}

/* Just a stub.  This method is optional and can safely be left
 unimplemented */
int utils_truncate(const char *path, off_t size)
{
	(void)path;
	(void)size;
	return 0;
}

/* Just a stub.  This method is optional and can safely be left
 unimplemented */
int utils_flush(const char *path, struct fuse_file_info *fi)
{
	(void)path;
	(void)fi;
	return 0;
}

/**
 * 通过分割路径，读取父目录的信息。p_dir_blk为父目录的第一块子文件的位置
 */
int dv_path(char *name, char *ext, const char *path, long *p_dir_blk, int flag)
{
	char *tmp_path, *p, *q;
	tmp_path = strdup(path);
	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));
	p = tmp_path;
	if (!p)
		return -errno;
	//跳过根目录/
	p++;

	q = strchr(p, '/');
	/**
	 *找到，
	 *说明是根目录下一个目录里的一个文件(不允许创建带/的目录)
	 *假如是创建目录，却找到了/,说明是二级以下目录，不允许
	 */
	// fslog("dvpath","%s74\n",tmp_path);
	if (flag == 2 && q != NULL)
		return -EPERM;
	else if (q != NULL)
	{			   // /目录/子目录 或 /目录/文件
		*q = '\0'; //截断tmp_path，tmp_path=目录static
		q++;
		p = q; // p=子目录 或 文件
		fslog("dvpath", "%s81\n", tmp_path);
		if (get_attr_from_open_pathblock(tmp_path, attr) == -1)
		{ //按路径读取这一目录，获取目录属性，确认目录有效
			fslog("dvpath", "%s83\n", tmp_path);
			free(attr);
			return -ENOENT;
		}
	}

	if (q == NULL)
	{ // /文件 或 /目录
		if (get_attr_from_open_pathblock("/", attr) == -1)
		{ //读取根目录属性，确认根目录有效
			fslog("dvpath", "%s90\n", path);
			free(attr);
			return -ENOENT;
		}
	}

	if (flag == 1)
	{ // /文件 或 /目录/文件
		q = strchr(p, '.');
		if (q != NULL)
		{ //存在拓展名
			*q = '\0';
			q++;
		}
	}
	fslog("dvpath", "%s102\n", path);
	//判断文件名，目录名或拓展名是否太长
	if (flag == 1)
	{
		fslog("flag", "%d\n", flag);
		if (strlen(p) > MAX_FILENAME + 1)
		{
			fslog("create", "namelong\n");
			free(attr);
			return -ENAMETOOLONG;
		}
		else if (strlen(p) > MAX_FILENAME)
		{
			if (*(p + MAX_FILENAME) != '~')
			{
				fslog("create", "namelong\n");
				free(attr);
				return -ENAMETOOLONG;
			}
		}
		else if (q != NULL)
		{
			if (strlen(q) > MAX_EXTENSION + 1)
			{
				fslog("create", "extlong");
				free(attr);
				return -ENAMETOOLONG;
			}
			else if (strlen(q) > MAX_EXTENSION)
			{
				if (*(q + MAX_EXTENSION) != '~')
				{
					fslog("create", "extlong\n");
					free(attr);
					return -ENAMETOOLONG;
				}
			}
		}
	}
	else if (flag == 2)
	{
		if (strlen(p) > MAX_FILENAME)
		{
			fslog("mkdir", "namelong\n");
			free(attr);
			return -ENAMETOOLONG;
		}
	}
	*name = *ext = '\0';
	if (p != NULL)
		strcpy(name, p);
	if (q != NULL)
		strcpy(ext, q);
	free(tmp_path);
	// printf("%s\n%s\n%s\n",dir,name,ext);
	//打开要创建的文件的上层目录
	*p_dir_blk = attr->nStartBlock;
	free(attr);
	if (*p_dir_blk == -1)
	{
		return -ENOENT;
	}
	return 0;
}

/**
 * 遍历目录下的所有文件和目录，如果已存在同名文件或目录，返回
 * 未处理多个块的情况
 */
int exist_check(struct u_fs_file_directory *file_dir, char *p, char *q,
				int *offset, int *pos, int size, int flag)
{
	//遍历目录下的所有文件和目录，如果已存在同名文件或目录，返回
	// int i=0;
	while (*offset < size)
	{
		if (flag == 0)
			*pos = *offset;
		else if (flag == 1 && file_dir->flag == 1 && strcmp(p, file_dir->fname) == 0 && ((*q == '\0' && strlen(file_dir->fext) == 0) || (*q != '\0' && strcmp(q, file_dir->fext) == 0)))
			return -EEXIST;
		else if (flag == 2 && file_dir->flag == 2 && strcmp(p, file_dir->fname) == 0)
			return -EEXIST;

		file_dir++;
		// i++;
		*offset += sizeof(struct u_fs_file_directory);
	}
	// fslog("exch","%d\n",i);
	return 0;
}

/**
 * 为上层目录增加一个后续块
 *
 */
int enlarge_blk(long p_dir_blk, struct u_fs_file_directory *file_dir,
				struct u_fs_disk_block *blk_info, long *tmp, char *p, char *q, int flag)
{
	long blk;
	tmp = malloc(sizeof(long));
	//找一个空闲块
	if (get_free_blocks(1, tmp) == 1)
		blk = *tmp;
	else
	{
		freePtrs(p, q, blk_info, NULL);
		return -errno;
	}
	// fslog("free","succeed182");
	free(tmp);
	//为上层目录添加一个后续块，扩大容量
	blk_info->nNextBlock = blk;
	//上层目录的blk_info域更新为容量大1的blk_info
	write_blkinfo_start_blk(p_dir_blk, blk_info);

	//设置新文件块信息（磁盘层）
	blk_info->size = sizeof(struct u_fs_file_directory);
	blk_info->nNextBlock = -1;
	//为新文件设置文件信息（文件层）
	file_dir = (struct u_fs_file_directory *)blk_info->data;
	init_file_dir(file_dir, p, q, flag);

	//为新文件找到起始块
	tmp = malloc(sizeof(long));
	if (get_free_blocks(1, tmp) == 1)
		file_dir->nStartBlock = *tmp;
	else
	{
		return -errno;
	}
	free(tmp);
	//将新文件块信息写入后续块（新文件所在块）中
	write_blkinfo_start_blk(blk, blk_info);

	//将新文件起始块信息写入对应块中
	new_emp_blk(blk_info);
	write_blkinfo_start_blk(file_dir->nStartBlock, blk_info);
	return 0;
}
/**
 * 在bitmap中标记块是否被使用
 *
 */
int utils_set_blk(long start_blk, int flag)
{

	if (start_blk == -1)
		return -1;
	//一个bit代表一个块的使用情况，一个byte代表8个块
	//小于8个块的部分，要移位后，操作第i个位
	int start = start_blk / 8;
	int left = start_blk % 8;
	int f;

	int mask = 1;
	mask <<= left;

	FILE *fp = NULL;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
		return -1;
	fseek(fp, FS_BLOCK_SIZE + start, SEEK_SET);
	int *tmp = malloc(sizeof(int));
	fread(tmp, sizeof(int), 1, fp);
	f = *tmp;
	if (flag) //将该位置1，其他位不动
		f |= mask;
	else
		//将该位置0，其他位不动
		f &= ~mask;

	*tmp = f;
	fseek(fp, FS_BLOCK_SIZE + start, SEEK_SET);
	fwrite(tmp, sizeof(int), 1, fp);
	fclose(fp);
	free(tmp);
	return 0;
}
/**
 * 初始化要创建的文件或目录
 */
void init_file_dir(struct u_fs_file_directory *file_dir, char *name, char *ext,
				   int flag)
{
	//设置要创建的文件或目录的名字
	strcpy(file_dir->fname, name);
	if (flag == 1 && *ext != '\0')
		strcpy(file_dir->fext, ext);
	file_dir->fsize = 0;
	file_dir->flag = flag;
}
/**
 * 初始化disk_block单元
 */
void new_emp_blk(struct u_fs_disk_block *blk_info)
{
	/* initialize the file or the directory block */
	blk_info->size = 0;
	blk_info->nNextBlock = -1;
	strcpy(blk_info->data, "\0");
}

/**
 * 判断该path中是否含有子目录或子文件
 */
int is_empty(const char *path)
{

	struct u_fs_disk_block *blk_info = malloc(sizeof(struct u_fs_disk_block));
	struct u_fs_file_directory *attr = malloc(
		sizeof(struct u_fs_file_directory));
	//读取属性到attr里
	if (get_attr_from_open_pathblock(path, attr) == -1)
	{
		freePtrs(blk_info, attr, NULL);
		return 0;
	}
	long start_blk;
	start_blk = attr->nStartBlock;
	if (attr->flag == 1)
	{ //传入路径为文件
		freePtrs(blk_info, attr, NULL);
		return 0;
	}
	if (get_blkinfo_from_read_blkpos(start_blk, blk_info) == -1)
	{ //读出块信息
		freePtrs(blk_info, attr, NULL);
		return 0;
	}

	struct u_fs_file_directory *file_dir =
		(struct u_fs_file_directory *)blk_info->data;
	int pos = 0;
	//遍历目录下文件，假如存在使用中的文件或目录，非空
	while (pos < blk_info->size)
	{

		if (file_dir->flag != 0)
		{
			freePtrs(blk_info, attr, NULL);
			return 0;
		}
		file_dir++;
		pos += sizeof(struct u_fs_file_directory);
	}

	freePtrs(blk_info, attr, NULL);
	return 1;
}

/**
 * 将attr属性赋值给path所指的文件属性
 */
int utils_setattr(const char *path, struct u_fs_file_directory *attr)
{
	int res;
	struct u_fs_disk_block *blk_info = malloc(sizeof(struct u_fs_disk_block));

	fslog("write", "%s:%d\n", path, attr->fsize);

	char *p = malloc(15 * sizeof(char)), *q = malloc(15 * sizeof(char));
	long start_blk;
	//拆分路径，找到父级目录块
	if ((res = dv_path(p, q, path, &start_blk, 1)))
	{
		freePtrs(p, q, NULL);
		return res;
	}
	fslog("setattr", "%d\n", start_blk);
	//读出该文件父目录信息
	if (get_blkinfo_from_read_blkpos(start_blk, blk_info) == -1)
	{
		res = -1;
		freePtrs(blk_info, NULL);
		return res;
	}
	struct u_fs_file_directory *file_dir =
		(struct u_fs_file_directory *)blk_info->data;
	int offset = 0;
	while (offset < blk_info->size)
	{
		//循环遍历块内容
		if (file_dir->flag != 0 && strcmp(p, file_dir->fname) == 0 && (*q == '\0' || strcmp(q, file_dir->fext) == 0))
		{ //找到该文件
			//设置为attr指定的属性
			set_file_dir(attr, file_dir);
			res = 0;
			fslog("set", "find and set\n");
			break;
		}
		//读下一个文件
		file_dir++;
		offset += sizeof(struct u_fs_file_directory);
	}
	freePtrs(p, q, NULL);
	//将修改后的目录信息写回磁盘文件
	if (write_blkinfo_start_blk(start_blk, blk_info) == -1)
	{
		res = -1;
	}
	fslog("set", "write back\n");
	freePtrs(blk_info, NULL);
	return res;
}
/**
 * The src_ attribute assignment to dest_
 * 读出src_的属性到dest_
 */
void set_file_dir(struct u_fs_file_directory *src_,
				  struct u_fs_file_directory *dest_)
{
	strcpy(dest_->fname, src_->fname);
	strcpy(dest_->fext, src_->fext);
	dest_->fsize = src_->fsize;
	dest_->nStartBlock = src_->nStartBlock;
	dest_->flag = src_->flag;
}

/**
 * From the beginning of the next_block block,
 * initialize blk_info block and the continued block
 * 从next_blk起清空blk_info后续块
 */
void nextClear(long next_blk, struct u_fs_disk_block *blk_info)
{
	while (next_blk != -1)
	{
		utils_set_blk(next_blk, 0);
		get_blkinfo_from_read_blkpos(next_blk, blk_info);
		next_blk = blk_info->nNextBlock;
	}
}
/**
 * Find the position according to the starting blocks and offset
 * 根据起始块和偏移量寻找位置
 * return 0 on success , -1 on failed.
 *
 * offset： 偏移量
 * start_blk: 起始块
 */
int find_off_blk(long *start_blk, long *offset, struct u_fs_disk_block *blk_info)
{
	while (1)
	{
		if (get_blkinfo_from_read_blkpos(*start_blk, blk_info) == -1)
		{
			return -errno;
		}
		// offset在当前块中，说明当前文件没有跨多个块，只要在当前块找offset即可
		if (*offset <= blk_info->size)
			break;
		//否则要在后续块中找offset，先减去当前块容量（当前文件跨了很多个块）
		*offset -= blk_info->size;
		*start_blk = blk_info->nNextBlock;
	}
	return 0;
}
