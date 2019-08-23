#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_NAME "/dev/x"

int g_fd;

void signal_handler(int signum)
{
	char data[1024] = {0};
	int len;
	//读取并输出 STDIN_FILENO 上的输入
	len = read(g_fd, &data, 1024);

	printf("catch %d signal, input data len:%d, data:%s\n", signum, len, data);
}

void usage(char *args)
{
	printf("%s devname\n", args);
}

int main(int argc, char **argv)
{
	int oflags;
	int fd;

	if (argc != 2) {
		usage(argv[1]);
		return 0;
	}

	fd = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		printf("open %s fail\n", argv[1]);
		return 0;
	}

	g_fd = fd;

	//绑定信号处理函数
	signal(SIGIO, signal_handler);
	//通过 F_SETOWN IO 控制命令设置设备文件的拥有者为本进程，
	//这样从设备驱动发出的信号才能被本进程接收到
	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(fd, F_GETFL);
	//通过 F_SETFL IO 控制命令设置设备文件支持 FASYNC，即异步通知模式
	fcntl(fd, F_SETFL, oflags | FASYNC);

	while (1);

	close(fd);

	return 0;
}
