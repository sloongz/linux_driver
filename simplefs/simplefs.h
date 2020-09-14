#ifndef _SIMPLE_FS_H_
#define _SIMPLE_FS_H_

const int SIMPLEFS_MAGIC = 0x12345678;
const int SIMPLEFS_DEFAULT_BLOCK_SIZE = 4 * 1024;
const int SIMPLEFS_ROOT_INODE_NUMBER = 1;

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
	unsigned int version;
	unsigned int magic;
	unsigned int block_size;
	unsigned int free_blocks;

	struct simplefs_inode root_inode;

    char padding[ (4 * 1024) - (4 * sizeof(unsigned int))];
};


#endif
