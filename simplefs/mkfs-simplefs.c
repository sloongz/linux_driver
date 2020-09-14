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
	ssize_t ret;
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
	sb.free_blocks = 0;

	ret = write(fd, (char *)&sb, sizeof(sb));
	if (ret != SIMPLEFS_DEFAULT_BLOCK_SIZE) {
		printf("wirte failed\n");
	}

	close(fd);

	return 0;
}
