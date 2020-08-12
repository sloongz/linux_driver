
改进一下char_driver3.c
- **增加lseek功能**
- **增加mutex锁**


**lseek功能**：

man lseek

```
off_t lseek(int fd, off_t offset, int whence);

DESCRIPTION
       The lseek() function repositions the offset of the open file associated 
       with the file descriptor fd to the argument offset according to the 
       directive whence as follows:

       SEEK_SET
              The offset is set to offset bytes.

       SEEK_CUR
              The offset is set to its current location plus offset bytes.

       SEEK_END
              The offset is set to the size of the file plus offset bytes.
```
fd 是要操作的文件描述符

offset 是相对于whence 基准的偏移量

whence 可以是 SEEK_SET（文件指针开始），SEEK_CUR（文件指针当前位置），SEEK_END为文件指针尾

也就是说这个函数可以操作文件读写位置。

**mutex锁：**

互斥锁， 保护临界区


https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver4.c

make 生成char_driver4.ko

以root权限插入char_driver4.ko模块


```
# insmod char_driver4.ko
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

写个应用测试lseek功能

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
char4_test.c

```
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
```
使用test3应用程序

```
usage: char4_test -args <x>
	args: s SEEK_SET
	args: c SEEK_CUR
	args: e SEEK_END
	x: offset

```
s表示lseek调用SEEK_SET
c表示lseek调用SEEK_CUR
e表示lseek调用SEEK_END
x 是偏移量

**测试lseek：**

```
# echo "123456789abcdef" > /dev/x
# cat /dev/x
123456789abcdef
```
测试SEEK_SET

```
# ./char4_test -s 0
read:123456789abcdef

# ./char4_test -s 1
read:23456789abcdef

# ./char4_test -s 2
read:3456789abcdef

# ./char4_test -s 3
read:456789abcdef

# ./char4_test -s 10
read:bcdef
```
读数据的位置和lseek设置后的一样

测试SEEK_END

```
# ./char4_test -e 0
read:
# ./char4_test -e -1
read:

# ./char4_test -e -2
read:f

# ./char4_test -e -3
read:ef

# ./char4_test -e -4
read:def

# ./char4_test -e -16
read:123456789abcdef

```
读数据的位置和lseek设置后的一样， 这里注意字符串结尾有\0字符

测试SEEK_CUR

```
# ./char4_test -c 0
read:123456789abcdef

# ./char4_test -c 1
read:23456789abcdef

# ./char4_test -c 3
read:456789abcdef

```
因为打开文件时偏移量就是在文件头，所以测试和预期一样。

通过测试程序验证的驱动中lseek的功能。


**测试mutex:**

编译时打开DEBUG_MUTEX宏，如果用户写入了“sleep”字符串就会引起10秒的睡眠

在驱动写函数的的锁之间加入了睡眠， 逻辑如下：

```
lock
操作临界区
睡眠
unlock
```


驱动的读函数读数据的时候同样有锁

```
lock
操作临界区
unlock
```
如果在写的同时去读，写函数还没有释放锁，读函数就会等待锁，逻辑如下：

```
写函数（）
{
lock
操作临界区      读函数（）
睡眠            {
.                   lock
.                   获取不到锁（因为写函数持有锁）
.                   .
unlock              .                
}                    获取到了锁（因为写函数释放了锁）
                    操作临界区
                    unlock
                }
```
测试

```
# echo "sleep" > /dev/x
```
没有返回在等待
在开一个终端

```
# cat /dev/x
```
也没有返回，也在等待， 过了一会cat返回了

```
sleep
```
测试结果和预期一样

本文只是简单涉及了同步机制，还有其它阻塞和非阻塞的机制需要研究。