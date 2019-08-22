#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <unistd.h>

#define DEV_NAME "/dev/x"

void usage()
{
	printf("usage: char5_test -args <x>\n");
	printf("	args: r read /dev/x buf\n");
	printf("		x: read len\n");
	printf("	args: w write /dev/x buf\n");
	printf("		x: write string\n");
	printf("	args: o test open /dev/x\n");
	printf("	args: c test close /dev/x\n");
}

int main(int argc, char **argv)
{
	int fd=0;
	int opt;
	char *optstring = "ocr:w:";
	int x;
	int ret;
	char buf[1024] = {0};
	int i;

	while ((opt = getopt(argc, argv, optstring)) != -1) {
		//printf("opt = %c\n", opt);
		//printf("optarg = %s\n", optarg);
		//printf("optind = %d\n", optind);
		//printf("argv[optind - 1] = %s\n\n",  argv[optind - 1]);

		switch (opt) {
			case 'r':
				fd = open(DEV_NAME, O_RDWR);
				if (fd < 0) {
					printf("open fail\n");
					break;
				}

				x = atoi(argv[optind-1]);	
				ret = read(fd, buf, x);
				if (ret < 0) {
					printf("read fail\n");
				}
				if (ret >=0) {
					for (i=0; i<ret; i++) {
						printf("%c", buf[i]);
					}
					printf("\n");
				}
				close(fd);

				break;
			case 'w':
				fd = open(DEV_NAME, O_RDWR);
				if (fd < 0) {
					printf("open fail\n");
					break;
				}

				memset(buf, 0, 1024);
				memcpy(buf, optarg, strlen(optarg));
				ret = write(fd, buf, strlen(optarg));
				if (ret < 0) {
					printf("write fail\n");
				}
				close(fd);
				break;
			case 'o':
				fd = open(DEV_NAME, O_RDWR);
				if (fd < 0) {
					printf("open fail\n");
					break;
				} else {
					printf("open success\n");
				}
				sleep(10);
				break;
			case 'c':
				close(fd);
				printf("close\n");
				break;
			default:
				usage();
		}

	}
	return 0;
}
