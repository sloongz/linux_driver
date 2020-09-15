#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "simplefs.h"

void usage()
{
	printf("Usage: mkfs-simplefs <device>");
}

int main(int argc, char **argv)
{
	int fd;
	int ret;
	int len, size;
	char *addr = NULL;
	struct simplefs_super_block sb;

	if (argc != 2) {
		usage();
		return 0;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error opening the device\n");
		return -1;
	}

	memset(&sb, 0, sizeof(sb));
	sb.version = 1;
	sb.magic = SIMPLEFS_MAGIC;
	sb.block_size = SIMPLEFS_DEFAULT_BLOCK_SIZE;
	sb.inodes_count = 1;
	sb.free_blocks = ~0;

	ret = write(fd, (char *)&sb, sizeof(sb));
	if (ret != SIMPLEFS_DEFAULT_BLOCK_SIZE) {
		printf("wirte failed, ret=%d\n", ret);
		size = SIMPLEFS_DEFAULT_BLOCK_SIZE - ret;
		addr = (char *)&sb + ret;
		while (ret) {	
			len = size;
			ret = write(fd, addr, len);
			printf("write ret=%d\n", ret);
			if (ret == len) {
				break;
			} else {
				len -= ret;
				addr += ret;
			}
		}
	}

	close(fd);

	return 0;
}
