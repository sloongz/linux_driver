
**非阻塞**

- O_NONBLOC和poll

非阻塞操作的进程在不能进行设备操作时，并不挂起，它或者放弃，或者不停地查询，直至可以进行操作为止。

**O_NONBLOCK**

应用程序打开文件时可以设置非阻塞的标识符

在驱动中，如果设备得不到资源就返回-EAGAIN

```
if(filp->f_flags & O_NONBLOCK) {
		    return -EAGAIN;
} else {
    条件不满足就阻塞
    if (wait_event_interruptible(等待队列头, 条件)) {
		return -ERESTARTSYS;
	}
}
```

**IO多路复用poll**

select、poll、 epoll都是IO多路复用的一类函数，意思是在一个线程中可以监听多个文件描述符， 

如果其中任意一个文件描述符可读或可写函数就返回一个掩码，指明状态。

如果所有的文件描述符都不可读或不可写， 此时函数阻塞，阻塞的时间可以设置。

驱动中是如何工作的：

假如应用层调用了select检查设备是否可读， 此时设备没有资源部可读，那么select就阻塞了， 

如果资源到位了，select马上返回状态。

实际上在内核中select调用了驱动的poll接口， poll中注册了一个等待队列， 而这个等待队列和read资源到位的等待队列是一个。

驱动中代码大概的样子

```
struct dev {
    ...
	wait_queue_head_t r_wait; //读阻塞等待队列头
    ...
};

//poll接口
static unsigned int x_poll(struct file *filp,struct socket *sock, poll_table *wait)
{
    unsigned int mask = 0;
    struct x_dev *dev = filp->private_data;//获得设备结构体指针
    ...
    poll_wait(filp,&dev->r_wait,wait);//加读等待队列头到poll_table
    ...
    if(...)//可读
        mask |= POLLIN | POLLRDNORM;
    ...
    return mask;
}

//read函数等待资源
static ssize_t x_read(struct file *filp, char __user *buf, size_t size,
			loff_t *f_pos)
{
    if(filp->f_flags & O_NONBLOCK) {
		    return -EAGAIN;
    } else {
         如果 资源没到位 就阻塞等待直到资源到位
         {
            if (wait_event_interruptible(dev->r_wait, 条件为真)) {
	    	    return -ERESTARTSYS;
	        }
	    }
    }		    

    
    有数据可以从设备缓存中读取
}

//read资源到位函数

static ssize_t x_write(struct file *filp, const char __user *buf,
			size_t size, loff_t *f_pos)
{
    将数据写入数据缓存
    
    如果 条件==真 {  //资源到位了
        //资源到位了就通知读操作可以读数据
        wake_up_interruptible(&dev->r_wait);
    }
}
```
 上面的代码是以读堵塞为例子，流程大概是这个样子
 
 1 读操作没有可用资源就阻塞；
 
 2 如果资源到位了唤醒等待队列；
 
 3 poll接口中也添加了等待队列，资源到位唤醒等待队列的同时也通知了poll注册的事件， 并改变poll的标志位；
 
 4 poll标志改变并告知应用层select函数去检查状态
 
 5 select检查状态后发现设备可以进行读操作， 那就读一下吧。
 
 6 应用程序读设备
 
 代码：
 https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver7.c
 
 修改char_driver6.c得到char_driver7.c
 
 

对应的写一个测试程序char7_test.c

```
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
```
测试程序先每隔1S读一次设备（共读10此），如何设备没有资源就立即返回， 然后在调用select监听设备什么时候可读写。

---

编译 插入模块 创建设备节点

测试

```
# ./char7_test
no data to read, 1 times
no data to read, 2 times
no data to read, 3 times
no data to read, 4 times
no data to read, 5 times
no data to read, 6 times
no data to read, 7 times
no data to read, 8 times
no data to read, 9 times
no data to read, 10 times
Poll fd can be write
Poll fd can be write
Poll fd can be write

```
运行char7_test, 前10次都阻塞， 之后设备缓存中没有数据， 可以向设备中写数据， select 返回设备可写。

之后向设备中写入数据

```
# echo "hello" > /dev/x
```
查看运行char7_test那个终端

```
# ./char7_test
no data to read, 1 times
no data to read, 2 times
no data to read, 3 times
no data to read, 4 times
no data to read, 5 times
no data to read, 6 times
no data to read, 7 times
no data to read, 8 times
no data to read, 9 times
no data to read, 10 times
Poll fd can be write
Poll fd can be write
Poll fd can be write
Poll fd can be write
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read
Poll fd can be write
Poll fd can be read

```

select检查设备结点即可读有可写。

结果和预期的逻辑一样。


