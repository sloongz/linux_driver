
- **信号**

信号是用来通知进程发生了异步事件，在软件层次上是对中断机制的一种模拟，一个进程不必通过任何操作来等待信号到达，事实上，进程也不知道信号到底什么时候到达。

信号和阻塞非阻塞的对比图：

![signal](https://github.com/sloongz/linux_driver/blob/master/Image/char_driver8_1.png?raw=true)
Linux中可用的信号：

```
$ kill -l
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
 6) SIGABRT	 7) SIGBUS	 8) SIGFPE	 9) SIGKILL	10) SIGUSR1
11) SIGSEGV	12) SIGUSR2	13) SIGPIPE	14) SIGALRM	15) SIGTERM
16) SIGSTKFLT	17) SIGCHLD	18) SIGCONT	19) SIGSTOP	20) SIGTSTP
21) SIGTTIN	22) SIGTTOU	23) SIGURG	24) SIGXCPU	25) SIGXFSZ
26) SIGVTALRM	27) SIGPROF	28) SIGWINCH	29) SIGIO	30) SIGPWR
31) SIGSYS	34) SIGRTMIN	35) SIGRTMIN+1	36) SIGRTMIN+2	37) SIGRTMIN+3
38) SIGRTMIN+4	39) SIGRTMIN+5	40) SIGRTMIN+6	41) SIGRTMIN+7	42) SIGRTMIN+8
43) SIGRTMIN+9	44) SIGRTMIN+10	45) SIGRTMIN+11	46) SIGRTMIN+12	47) SIGRTMIN+13
48) SIGRTMIN+14	49) SIGRTMIN+15	50) SIGRTMAX-14	51) SIGRTMAX-13	52) SIGRTMAX-12
53) SIGRTMAX-11	54) SIGRTMAX-10	55) SIGRTMAX-9	56) SIGRTMAX-8	57) SIGRTMAX-7
58) SIGRTMAX-6	59) SIGRTMAX-5	60) SIGRTMAX-4	61) SIGRTMAX-3	62) SIGRTMAX-2
63) SIGRTMAX-1	64) SIGRTMAX	
```
写一个小程序测试一下：
char8_test1.c
```
#include <stdio.h>
#include <signal.h>

//typedef void (*sighandler_t)(int);
//sighandler_t signal(int signum, sighandler_t handler);

void signal_handler(int signum)
{
	printf("catch %d signal\n", signum);
}

int main()
{
	signal(SIGINT, signal_handler);
	while (1);
	return 0;
}
```
程序捕获SIGINT信号， SIGINT信号是终端中断的信号， 可以通过Ctrl + C发送给程序。

测试：
运行

```
$ ./char8_test1
```
程序进入while(1)无限循环

在此终端上按Crtl + C

```
$ ./char8_test1
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal

```
可以看到程序捕获了SIGINT信号。

但程序又无法终止了， kill 杀掉

```
$ ps aux
ubuntu     7488 99.4  0.0   4352   776 pts/17   R+   01:45   6:48 ./char8_test1

$ kill 7488
```
```
$ ./char8_test1
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal
^Ccatch 2 signal
Terminated
```

再写一个测试程序

可以获取标准输入输出IO上的信号
char8_test2.c 

```
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
```
运行程序，输入hello world , 程序会获取信号，测试结果：

```
$ ./char8_test2
hello world
catch 29 signal, input data len:12, data:hello world


```
运行程序，输入“hello world”，信号处理程序就抓取了输入信号。

总结：

用户程序需要三部来获取设备驱动的信号：

1.通过 F_SETOWN 设置设备文件的拥有者为本进程，fcntl(设备文件, F_SETOWN, getpid()),没有这一步没和不知道将信号发给那个进程；
2. 通过 F_SETFL 设置设备文件支持FASYNC（异步通知）；
3. 通过 signal()函数绑定信号和信号处理函数。
 

用户空间和内核交互：
![signal](https://github.com/sloongz/Tools/blob/master/image/char_driver8_2.png?raw=true)

用户空间获取设备信号需要3步， 内核空间也对应着3步

1. 当发出 F_SETOWN, 什么都没发生, 除了一个值被赋值给filp->f_owner；
2. 当 F_SETFL 被执行来打开 FASYNC, 驱动的 fasync 方法被调用 fasync_helper()；
3. 当数据到达, 所有的注册异步通知的进程必须被发出一个注册的信号 kill_fasync()。


驱动中信号使用模板:

```
struct dev {
    ...
	struct fasync_struct *async_queue; //异步结构体指针
    ...
};

static int x_w_fasync(int fd, struct file *filp, int mode)
{
    struct dev *dev = filp->private_data;
    return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static ssize_t x_write(struct file *filp, const char __user *buf, size_t 
        count,loff_t *f_pos)
{
    kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
}
```

在char_driver7.c的基础上新建char_driver8.c，代码
https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver8.c


写一个char8_test.c测试程序

Makefile

```
SOURCE = $(wildcard *.c)
TARGETS = $(patsubst %.c, %, $(SOURCE))
		 
CC = gcc
CFLAGS = -Wall -g
		 
all:$(TARGETS)
$(TARGETS):%:%.c
	$(CC) $< $(CFLAGS) -o $@
								 
.PHONY:clean all
clean:
	rm -rf $(TARGETS) cscope.* tags 

```
char8_test.c

```
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

```


---
测试

编译 插入模块 创建设备文件

运行char8_test

```
# ./char8_test /dev/x
```
等待

再开一个终端

```
# echo "hello world" > /dev/x
```

test7返回

```
# ./test7 /dev/x
catch 29 signal, input data len:12, data:hello world

```
应用程序等到了驱动发信号，测试和预期的结果一样。