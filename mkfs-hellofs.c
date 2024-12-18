#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "hellofs.h"

int main(int argc, char *argv[]) {
    int fd, sts;
    ssize_t ret;
    uint64_t welcome_inode_no;
    uint64_t welcome_data_block_no_offset;
    struct stat st;
    // construct superblock
    struct hellofs_superblock hellofs_sb = {
        .version = 1,
        .magic = HELLOFS_MAGIC,
        .blocksize = HELLOFS_DEFAULT_BLOCKSIZE,
        .inode_table_size = HELLOFS_DEFAULT_INODE_TABLE_SIZE,
        .inode_count = 2,
        .data_block_table_size = HELLOFS_DEFAULT_DATA_BLOCK_TABLE_SIZE,
        .data_block_count = 2,
        .flags = 0,
        .misc = 0,
    };

    sts = stat(argv[1],&st);
    if ( sts < 0 )
    {
        fprintf(stderr,"Unable to stat '%s': %s\n", argv[1], strerror(errno));
        return -1;
    }
    hellofs_sb.fs_size = st.st_size;
    
    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Error opening the device");
        return -1;
    }

    // construct inode bitmap
    char inode_bitmap[hellofs_sb.blocksize];
    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    inode_bitmap[0] = 1;

    // construct data block bitmap
    char data_block_bitmap[hellofs_sb.blocksize];
    memset(data_block_bitmap, 0, sizeof(data_block_bitmap));
    data_block_bitmap[0] = 1;

    // construct root inode
    struct hellofs_inode root_hellofs_inode = {
        .mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH,
        .inode_no = HELLOFS_ROOTDIR_INODE_NO,
        .data_block_no 
            = HELLOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hellofs_sb)
                + HELLOFS_ROOTDIR_DATA_BLOCK_NO_OFFSET,
        .dir_children_count = 1,
    };

    // construct welcome file inode
    char welcome_body[] = "Welcome Hellofs!!\n";
    welcome_inode_no = HELLOFS_ROOTDIR_INODE_NO + 1;
    welcome_data_block_no_offset = HELLOFS_ROOTDIR_DATA_BLOCK_NO_OFFSET + 1;
    struct hellofs_inode welcome_hellofs_inode = {
        .mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
        .inode_no = welcome_inode_no,
        .data_block_no 
            = HELLOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hellofs_sb)
                + welcome_data_block_no_offset,
        .file_size = sizeof(welcome_body),
    };

    // construct root inode data block
    struct hellofs_dir_record root_dir_records[] = {
		{
			.filename = "wel_helo.txt",
			.inode_no = welcome_inode_no,
		},
    };

    ret = 0;
    do {
        // write super block
        printf("Writing %ld byte superblock at file offset 0x%lX\n", sizeof(hellofs_sb), lseek(fd,0,SEEK_CUR));
        if (sizeof(hellofs_sb)
                != write(fd, &hellofs_sb, sizeof(hellofs_sb))) {
            ret = -1;
            break;
        }
        if ((off_t)-1
                == lseek(fd, hellofs_sb.blocksize, SEEK_SET)) {
            ret = -2;
            break;
        }

        // write inode bitmap
        printf("Writing %ld byte inode bitmap at file offset 0x%lX\n", sizeof(inode_bitmap), lseek(fd,0,SEEK_CUR));
        if (sizeof(inode_bitmap)
                != write(fd, inode_bitmap, sizeof(inode_bitmap))) {
            ret = -3;
            break;
        }

        // write data block bitmap
        printf("Writing %ld byte data block bitmap at file offset 0x%lX\n", sizeof(data_block_bitmap), lseek(fd,0,SEEK_CUR));
        if (sizeof(data_block_bitmap)
                != write(fd, data_block_bitmap,
                         sizeof(data_block_bitmap))) {
            ret = -4;
            break;
        }

        // write root inode
        printf("Writing %ld byte root inode at file offset 0x%lX\n", sizeof(root_hellofs_inode), lseek(fd,0,SEEK_CUR));
        if (sizeof(root_hellofs_inode)
                != write(fd, &root_hellofs_inode,
                         sizeof(root_hellofs_inode))) {
            ret = -5;
            break;
        }

        // write welcome file inode
        printf("Writing %ld byte welcome inode at file offset 0x%lX\n", sizeof(welcome_hellofs_inode), lseek(fd,0,SEEK_CUR));
        if (sizeof(welcome_hellofs_inode)
                != write(fd, &welcome_hellofs_inode,
                         sizeof(welcome_hellofs_inode))) {
            ret = -6;
            break;
        }

        // write root inode data block
        if ((off_t)-1
                == lseek(
                    fd,
                    HELLOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hellofs_sb)
                        * hellofs_sb.blocksize,
                    SEEK_SET)) {
            ret = -7;
            break;
        }
        printf("Writing %ld byte root dir records (%ld entries) at file offset 0x%lX\n", sizeof(root_dir_records), sizeof(root_dir_records)/sizeof(struct hellofs_dir_record), lseek(fd,0,SEEK_CUR));
        if (sizeof(root_dir_records) != write(fd, root_dir_records, sizeof(root_dir_records))) {
            ret = -8;
            break;
        }

        // write welcome file inode data block
        if ((off_t)-1
                == lseek(
                    fd,
                    (HELLOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hellofs_sb)
                        + 1) * hellofs_sb.blocksize,
                    SEEK_SET)) {
            ret = -9;
            break;
        }
        printf("Writing %ld byte welcome body at file offset 0x%lX\n", sizeof(welcome_body), lseek(fd,0,SEEK_CUR));
        if (sizeof(welcome_body) != write(fd, welcome_body,
                                          sizeof(welcome_body))) {
            ret = -10;
            break;
        }
    } while (0);

    close(fd);
    return ret;
}
