/*
 FUSE: Filesystem in Userspace
 Copyright (C) 2016  Nango <luonango@qq.com>

 This program can be distributed under the terms of the GNU GPL.
 See the file COPYING.
 */
/***
 * author: nango
 * using: init blocks of super_blocks,bitmap_blocks and the data_blocks
 * start: 2016-02-09 21:00
 * finished:
 *
 *
 ***/
#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *vdisk_path = "/home/hadoop/code/homework/newdisk";
char *root_path = "/";
/////////////////////////////////////////////////////
int init_sb_bitmap_data_blocks();

/*******************************************************************************/
/*******************************************************************************/
int main()
{
	init_sb_bitmap_data_blocks();
	printf("init_sb_bitmap_data_blocks   all initial success!\n");
	return 0;
}
/*******************************************************************************/
/*******************************************************************************/

int init_sb_bitmap_data_blocks()
{
	FILE *fp = NULL;
	fp = fopen(vdisk_path, "r+");
	if (fp == NULL)
	{
		printf(
			"please create newdisk on path!\nvia:\n\tdd bs=1K count=5K if=/dev/zero of=diskimg\n");

		return 0;
	}

	printf("init_sb_bitmap_data_blocks   open disk %s success!\n", vdisk_path);

	struct super_block *super_blk = malloc(sizeof(struct super_block));

	/*initial super_block*/
	fseek(fp, 0, SEEK_END); //计算磁盘包含的块数目
	//5242880byte/512byte=10240
	super_blk->fs_size = ftell(fp) / FS_BLOCK_SIZE; //磁盘块数目
	super_blk->first_blk = 1 + BITMAP_BLOCK;		//1为sb块，其中根目录在bitmap block后（1282)
	super_blk->bitmap = BITMAP_BLOCK;

	if (fseek(fp, 0, SEEK_SET) != 0) //将super block写回磁盘，完成初始化
		fprintf(stderr, "failed!\n");
	fwrite(super_blk, sizeof(struct super_block), 1, fp);

	printf("init_sb_bitmap_data_blocks   initial super_block success!\n");

	/*initial Bitmap_block*/
	if (fseek(fp, FS_BLOCK_SIZE * 1, SEEK_SET) != 0)
		fprintf(stderr, "failed!\n");
	//在bitmap中记录bitmap_block和super_block占用情况(一共记录1281bits)
	int a[40];			  //代表1280块占用情况                      //bitmap 占BITMAP_LENGTH块block，super_block占1块，root占一块
	memset(a, -1, 40);	  //-1为32个1                  //其中一个bit代表一块 32*40=1280
	fwrite(a, 40, 1, fp); //置位前1280个(0_1279)bit， 1280尚未置位1

	//置位1280和1281位，此时一个int有32位
	int f = 0; //1280-1311个bit

	int mask = 1; //1280
	mask <<= 1;
	f |= mask;

	mask <<= 2; //1281
	f |= mask;
	fwrite(&f, sizeof(int), 1, fp);
	//bitmap剩下块记录为空闲块0
	int b[87];			  //bitmap 有40+1+87=1280块
	memset(b, 0, 87);	  //0为32个0
	fwrite(b, 87, 1, fp); //填充bitmap后续block

	//在data_block中，全部记录为空闲块
	//后续1280-1个block置为0，对5M磁盘，只用到前面10240bit（2.5个block）
	int total = (BITMAP_BLOCK - 1) * FS_BLOCK_SIZE;
	char rest[total];
	memset(rest, 0, total);
	fwrite(rest, total, 1, fp);

	printf("init_sb_bitmap_data_blocks   initial bitmap_block success!\n");

	/*  initialize the root directory block */
	fseek(fp, FS_BLOCK_SIZE * (BITMAP_BLOCK + 1), SEEK_SET);
	struct u_fs_disk_block *root = malloc(sizeof(struct u_fs_disk_block));
	root->size = 0;
	root->nNextBlock = -1;
	root->data[0] = '\0';
	fwrite(root, sizeof(struct u_fs_disk_block), 1, fp); //写入磁盘，初始化完成

	printf("init_sb_bitmap_data_blocks   initial root_directory success!\n");

	if (fclose(fp) != 0)
		printf("init_sb_bitmap_data_blocks  error :close newdisk failed !\n");

	return 0;
}
