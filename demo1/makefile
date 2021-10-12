all:init_sb_bitmap_data_blocks u_fs
init_sb_bitmap_data_blocks:init_sb_bitmap_data_blocks.o
	gcc    init_sb_bitmap_data_blocks.o -o init_sb_bitmap_data_blocks
u_fs:u_fs.o utils.o
	gcc u_fs.o utils.o -o u_fs -Wall -D_FILE_OFFSET_BITS=64 -g -pthread -lfuse -lrt -ldl
u_fs.o:global.h u_fs.c
	gcc  -Wall -D_FILE_OFFSET_BITS=64 -g   -c -o u_fs.o u_fs.c
utils.o:utils.h utils.c
	gcc  -Wall -D_FILE_OFFSET_BITS=64 -g   -c -o utils.o utils.c
init_sb_bitmap_data_blocks.o:global.h init_sb_bitmap_data_blocks.c
	gcc  -Wall -D_FILE_OFFSET_BITS=64 -g   -c -o init_sb_bitmap_data_blocks.o init_sb_bitmap_data_blocks.c
.PHONY : all
clean :
	rm  -f u_fs init_sb_bitmap_data_blocks u_fs.o utils.o init_sb_bitmap_data_blocks.o
