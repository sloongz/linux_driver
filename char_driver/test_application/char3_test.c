#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_NAME "/dev/x"

#define G_MEM_SIZE 1024

#define X_MAGIC   'X'
#define X_MAX_NR  4
#define X_IO_SET	_IO(X_MAGIC, 0)
#define X_IO_CLEAR	_IO(X_MAGIC, 1)
#define X_IOR		_IOR(X_MAGIC, 2, char[G_MEM_SIZE])
#define X_IOW		_IOW(X_MAGIC, 3, char[G_MEM_SIZE])

static char g_buf[G_MEM_SIZE];

void usage()
{
	printf("usage: char3_test <x>\n");
	printf("x: w - write dev buf\n");
	printf("   r - read dev buf\n");
	printf("   s - set dev buf 's'\n");
	printf("   c - clear dev buf\n");
}

int main(int argv, char **argc)
{
	int fd;
	int ret;

	if (argv != 2) {
		usage();
		return 0;
	}
	memset(g_buf, 0, sizeof(g_buf));

	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		printf("open error\n");
		return 0;
	}

	switch (argc[1][0]) {
		case 'w':
			sprintf(g_buf, "%s", "app write data used ioctl\n");
				ret = ioctl(fd, X_IOW, g_buf);
			if (ret < 0) 
			  printf("ioctl X_IOW fail\n");
			break;
		case 'r':
			ret = ioctl(fd, X_IOR, g_buf);
			if (ret < 0) 
			  printf("ioctl X_IOR fail\n");
			else
			  printf("read data form kernel: %s", g_buf);
			break;
		case 's':
			ret = ioctl(fd, X_IO_SET, g_buf);
			if (ret < 0) 
			  printf("ioctl X_IO_SET fail\n");
			break;
		case 'c':
			ret = ioctl(fd, X_IO_CLEAR, g_buf);
			if (ret < 0) 
			  printf("ioctl X_IO_CLEAR fail\n");
			break;
		default:
			usage();
	}

	close(fd);

	return 0;
}
