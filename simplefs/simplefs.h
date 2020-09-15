#ifndef _SIMPLE_FS_H_
#define _SIMPLE_FS_H_

const uint64_t SIMPLEFS_MAGIC = 0x12345678;
const int SIMPLEFS_DEFAULT_BLOCK_SIZE = 4 * 1024;
const int SIMPLEFS_ROOTDIR_INODE_NUMBER = 1;
const int SIMPLEFS_INODESTORE_BLOCK_NUMBER = 1;


struct simplefs_inode {
	mode_t mode;
	uint32_t inode_no;
	uint32_t data_block_number;

	union {
		uint32_t file_size;
		uint32_t dir_children_count;
	};
};

struct simplefs_super_block {
	uint64_t version;
	uint64_t magic;
	uint64_t block_size;
	uint64_t free_blocks;
	uint64_t inodes_count; 

    char padding[(4*1024) - (5 * sizeof(uint64_t))];
};


#endif
