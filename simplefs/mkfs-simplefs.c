#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "simplefs.h"

/*
 * superblock 占用一个block-------------------------(block 0)
 *		version=1 
 *		magic=0x12345678 
 *      inodes_count=2  (root_inode and welcome file inode) 
 * =============================================================  
 *
 * 相当于 inode table, 占用一个block ---------------(block 1)
 * root_inode
 *     inode_no = 1
 *     data_block_number = 2 
 *     dir_children_count = 1
 * welcome file inode
 *     inode_no = 2
 *	   data_block_number = 3
 * ============================================================
 *
 * 接下来是data block 
 *
 * root inode 对应的 dir 占用一个block ------------(block 2)
 *    写入文件名和文件对应的inode号 
 *    [file name | inode number]
 *    [file name | inode number]
 *    ...
 *
 * welcome file 文件内容占用一个block -------------(block 3)
 *    文件内容
 *
*/


const uint64_t WELCOMEFILE_DATABLOCK_NUMBER = 3;
const uint64_t WELCOMEFILE_INODE_NUMBER = 2;

void usage()
{
	printf("Usage: mkfs-simplefs <device>");
}

int data_write(int fd, char *addr, int len)
{
	int ret;
	int size;
	int length = len;

	do {
		ret = write(fd, addr, length);
		if (ret>=0) {
			length -= ret;
			addr += ret; 
		} else {
			return -1;
		}
	} while (length);

	return 0;
}

static int write_superblock(int fd)
{
	int ret;
	struct simplefs_super_block sb;

	memset(&sb, 0, sizeof(sb));
	sb.version = 1;
	sb.magic = SIMPLEFS_MAGIC;
	sb.block_size = SIMPLEFS_DEFAULT_BLOCK_SIZE;
	//1.root_dir 2.welcom file
	sb.inodes_count = 2;
	sb.free_blocks = ~0 & (1<<WELCOMEFILE_DATABLOCK_NUMBER);

	ret = data_write(fd, (char *)&sb, sizeof(sb));
	if (!ret)
		printf("write superblock success\n");

	return 0;
}

int write_root_inode(int fd)
{
	int ret;
	struct simplefs_inode root_inode;

	root_inode.mode = S_IFDIR;
	root_inode.inode_no = SIMPLEFS_ROOTDIR_INODE_NUMBER;
	root_inode.data_block_number = SIMPLEFS_ROOTDIR_DATABLOCK_NUMBER;
	root_inode.dir_children_count = 1;

	ret = data_write(fd, (char *)&root_inode, sizeof(root_inode));
	if (ret < 0) {
		printf("write root inode failed\n");
		return -1;
	}

	printf("write root inode success\n");

	return 0;
}

int write_inode(int fd, const struct simplefs_inode *i, int offset)
{
	int ret;
	off_t nbytes;

	ret = data_write(fd, (char *)i, sizeof(*i));
	if (ret < 0) {
		printf("write inode failed\n");
		return -1;
	}

	nbytes = SIMPLEFS_DEFAULT_BLOCK_SIZE - sizeof(*i)*(1+offset);	
	ret = lseek(fd, nbytes, SEEK_CUR);

	printf("write %d inode table success\n", 1+offset);

	return 0;
}

int write_dirent(int fd, const struct simplefs_dir *dirent)
{
	int ret;
	off_t nbytes;

	ret = data_write(fd, (char *)dirent, sizeof(*dirent));
	if (ret < 0) {
		printf("write dirent failed\n");
		return -1;
	}

	nbytes = SIMPLEFS_DEFAULT_BLOCK_SIZE - sizeof(*dirent);
	ret = lseek(fd, nbytes, SEEK_CUR);

	printf("write dirent success\n");

	return 0;
}

int write_block(int fd, char *block, int len)
{
	int ret;

	ret = data_write(fd, (char *)block, len);	
	if (ret < 0) {
		printf("write data block failed\n");
		return -1;
	}

	printf("write data block success");
	return 0;
}

int main(int argc, char **argv)
{
	int fd;

	if (argc != 2) {
		usage();
		return 0;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error opening the device\n");
		return -1;
	}

	write_superblock(fd);

	write_root_inode(fd);

	char welcome_file[] = "hello world";
	struct simplefs_inode welcome = {
		.mode = S_IFREG,
		.inode_no = WELCOMEFILE_INODE_NUMBER,
		.data_block_number = WELCOMEFILE_DATABLOCK_NUMBER,
		.file_size = sizeof(welcome_file),
	};
	write_inode(fd, &welcome, 1); //offset=1, 1=root inode 2=welcome inode

	struct simplefs_dir wel_dir = {
		.filename = "hello",
		.inode_no = WELCOMEFILE_INODE_NUMBER,
	};
	write_dirent(fd, &wel_dir);

	write_block(fd, welcome_file, welcome.file_size);

	close(fd);

	return 0;
}
