
改进一下，char_driver3.c
- **增加ioctl功能**

 设备驱动程序除了读写设备外，大部分驱动还需要一种管理和控制设备的功能，
 这些操作通常通过ioctl方法支持。
 
 用户空间接口：
 
```
#include <sys/ioctl.h>
int ioctl(int d, int request, ...);
```

内核空间接口（2.6.36之后内核修改了接口）：

```
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
    int (*ioctl)(stuct file *filp, unsigned int cmd, unsigned long arg);
#else
    int (*ioctl)(struct inode *inode_p, struct file *filp, unsigned int cmd, unsigned long args);
#endif
```
内核空间函数实现

- filp对应着文件描述符
- cmd由用户空间不经修改的传入内核空间
- arg无论用户空间使用的是指针还是整数值，它都以unsigned long的形式传递给驱动程序，
如果应用调用程序没有传递第三个参数，驱动程序接收的arg就处于未定义状态

编写ioctl需要预先统一用户空间和内核空间的cmd编号，编号不是随便填写的，需要根据内核约定的方法为驱动程序选择cmd编号。

Documentation/ioctl-number.txt文件中罗列了内核使用的幻数，在选择自己的幻数的时候要避免和内核冲突

include/uapi/asm-generic/ioctl.h中定义了要使用的位字段：

```
#define _IOC(dir,type,nr,size) \
	(((dir)  << _IOC_DIRSHIFT) | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT) | \
	 ((size) << _IOC_SIZESHIFT))
```
- type 幻数，选择一个号码并在整个驱动中使用这个号码。8bit字宽。
- number  序数，8bit字宽。
- direction 如果相关的命令涉及到数据传输，则此字段定义了数据的传输方向。可以使用的值：

```
_IOC_NONE  没有数据传输
_IOC_READ  从设备读数据
_IOC_WRITE  往设备写数据
_IOC_READ | _IOC_WRITE  双向数据传输

```
- size  用户数据大小

在include/uapi/asm-generic/ioctl.h还定义了一些构造命令的宏，可以方便用户使用

```
/* used to create numbers */
#define _IO(type,nr)		_IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)	_IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOR_BAD(type,nr,size)	_IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW_BAD(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR_BAD(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

/* used to decode ioctl numbers.. */
#define _IOC_DIR(nr)		(((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)		(((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)		(((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)		(((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

/* ...and for the drivers/sound files... */

#define IOC_IN		(_IOC_WRITE << _IOC_DIRSHIFT)
#define IOC_OUT		(_IOC_READ << _IOC_DIRSHIFT)
#define IOC_INOUT	((_IOC_WRITE|_IOC_READ) << _IOC_DIRSHIFT)
#define IOCSIZE_MASK	(_IOC_SIZEMASK << _IOC_SIZESHIFT)
#define IOCSIZE_SHIFT	(_IOC_SIZESHIFT)
```

https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver3.c

make 生成char_driver3.ko

写一个用户测试程序，程序调用ioctl完成驱动中定义的4种操作
- X_IOW 向内核驱动中写入一段字符串
- X_IOR 从内核驱动中读取一段字符串
- X_IO_SET 将驱动中的buf全部置成字符‘s’
- X_IO_CLEAR 将驱动中的buf全部清0


Makefile

```
OUT=char3_test
objects = char3_test.o
#ARM_CC=arm-linux-gnueabihf-gcc
#CC=$(ARM_CC)
CC=gcc
$(OUT): $(objects)
	$(CC) -lpthread -o $@ $(objects)   
$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:clean
clean:
	rm -rf $(OUT) $(objects) cscope.* tags
```
char_test3.c

```
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
	printf("usage: test <x>\n");
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

```


以root权限插入模块

```
insmod char_drivdr3.ko
```
查看设备

```
$cat /proc/devices
Character devices:
...
222 char_driver
...
```
创建设备结点

```
# mknod /dev/x c 222 0

```
测试设备

```
# echo "12345678" > /dev/x
# cat /dev/x
12345678
```
读写正常

使用test2应用程序

```
# ./char3_test 
usage: char3_test <x>
x: w - write dev buf
   r - read dev buf
   s - set dev buf 's'
   c - clear dev buf
```

---
X_IO_CLEAR 将驱动中的buf全部清0

```
# echo "12345678" > /dev/x
# ./char3_test c
# cat /dev/x
```
可以看到调用X_IO_CLEAR把驱动的buf清0

---
X_IO_SET 将驱动中的buf全部置成字符‘s’


```
# ./char3_test s
# cat /dev/x
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
```
可以看到调用X_IO_SET把驱动的buf全置‘s’


---

X_IOW 向内核驱动中写入一段字符串


```
# ./char3_test w
```
dmesg查看

```
[16226.009301] x_ioctl X_IOW
[16226.009303] app data:app write data used ioctl
```
cat 出来

```
# cat /dev/x
app write data used ioctl
```
可以看到调用X_IOW向驱动的buf中写入了数据

---
X_IOR 从内核驱动中读取一段字符串

```
# echo "hello" > /dev/x
# ./char3_test r
read data form kernel: hello

```
可以看到调用X_IOR从驱动的buf中读出了数据

ps: 本例子只是个测试， ioctl用来控制和管理设备，最好不要用来传输数据。

char_driver1、char_driver2、char_driver3都没有考虑临界资源问题，比如同时读写设备会同时操作同一片内存，

就会出现数据混乱，需要改进。



