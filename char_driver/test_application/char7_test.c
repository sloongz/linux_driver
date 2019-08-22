#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define DEV_NAME "/dev/x"

int main()
{
	int fd;
	int ret;
	int i;
	char buf[1024] = {0};
	//struct timeval {
	//	time_t         tv_sec;     /* seconds */
	//	suseconds_t    tv_usec;    /* microseconds */
	//};
	struct timeval tv;
	fd_set rfds, wfds;	//读/写文件描述符集


	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		printf("open fail\n");
		return -1;
	}

	//设置O_NONBLOCK位
	int flags=fcntl(fd,F_GETFL,0);
	flags|=O_NONBLOCK;
	fcntl(fd,F_SETFL,flags);

	for (i=0; i<10; i++) {
		memset(buf, 0, sizeof(buf));
		ret = read(fd, buf, sizeof(buf));
		if (ret < 0) {
			printf("no data to read, %d times\n", i+1);
		} else {
			printf("read:%s\n", buf);
		}
		sleep(1);
	}

	tv.tv_sec = 3;//3秒不能读写，select就返回0 表示超时
	tv.tv_usec = 0;

	while (1) {

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(fd, &rfds);
		FD_SET(fd, &wfds);

		ret = select(fd + 1, &rfds, &wfds, NULL, &tv);
		//ret = select(fd + 1, &rfds, &wfds, NULL, NULL);
		if (ret < 0) {
			printf("select error\n");
		} else if (ret > 0) {
			if (FD_ISSET(fd, &wfds)) {
				printf("Poll fd can be write\n");
			}
			if (FD_ISSET(fd, &rfds)) {
				printf("Poll fd can be read\n");
			}
		} else {
			printf("No data timeout\n");
		}
		sleep(5);//5秒检查一次
	}


	close(fd);

	return 0;
}
