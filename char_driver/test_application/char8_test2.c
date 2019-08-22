#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void signal_handler(int signum)
{
	char data[1024] = {0};
	int len;
	//读取并输出 STDIN_FILENO 上的输入
	len = read(STDIN_FILENO, &data, 1024);

	printf("catch %d signal, input data len:%d, data:%s\n", signum, len, data);
}


int main()
{
	int oflags;

	//绑定信号处理函数
	signal(SIGIO, signal_handler);
	//通过 F_SETOWN IO 控制命令设置设备文件的拥有者为本进程，
	//这样从设备驱动发出的信号才能被本进程接收到
	fcntl(STDIN_FILENO, F_SETOWN, getpid());
	oflags = fcntl(STDIN_FILENO, F_GETFL);
	//通过 F_SETFL IO 控制命令设置设备文件支持 FASYNC，即异步通知模式
	fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC);

	while (1);

	return 0;
}
