#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#define DEV_NAME "/dev/x"

void usage()
{
	printf("usage: char4_test -args <x>\n");
	printf("	args: s SEEK_SET\n");
	printf("	args: c SEEK_CUR\n");
	printf("	args: e SEEK_END\n");
	printf("	x: offset\n");
}

int main(int argc, char **argv)
{
	int fd;
	int opt;
	char *optstring = "s:c:e:";
	int x;
	int ret;
	char buf[1024] = {0};

	if (argc != 3) {
	  usage();
	  return 0;
	}

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		//printf("opt = %c\n", opt);
		//printf("optarg = %s\n", optarg);
		//printf("optind = %d\n", optind);
		//printf("argv[optind - 1] = %s\n\n",  argv[optind - 1]);

		fd = open(DEV_NAME, O_RDWR);
		if (fd < 0) {
		  printf("open fail\n");
		  break;
		}
		switch (opt) {
		case 's':
			x = atoi(argv[optind-1]);	
			ret = lseek(fd, x, SEEK_SET);
			if (ret < 0) {
				printf("lseek fail\n");
				break;
			}
			break;
		case 'c':
			x = atoi(argv[optind-1]);
			ret = lseek(fd, x, SEEK_CUR);
			if (ret < 0) {
				printf("lseek fail\n");
				break;
			}
			break;
		case 'e':
			x = atoi(argv[optind-1]);
			ret = lseek(fd, x, SEEK_END);
			if (ret < 0) {
				printf("lseek fail\n");
				break;
			}
			break;
		default:
			usage();
		}
	
		ret = read(fd, buf, 1024);
		if (ret < 0)
			printf("read fail\n");
		else
			printf("read:%s\n", buf);

		close(fd);
	}
	return 0;
}
