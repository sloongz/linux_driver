
**并发控制**


同步和互斥是操作系统的经典问题：

同步：多个进程之间相互协作， 共同完成任务；

互斥：为多个进程分时使用有限的资源

信号量在操作系统中一般是一个整数变量，假如值为S
- S > 0：S表示可用资源的个数
- S = 0：表示无可用资源， 无等待进程
- S < 0：表示等待队列中的进程个数

用信号量PV操作（PV操作不可中断）实现的互斥同步：

- P操作： S = S - 1 表示申请一个资源
- V操作：S = S + 1 表示释放一个资源


```
p {
    S = S - 1;
    if (S >= 0)
        进程继续执行
    else
        进程置为等待状态，排入等待队列
}

V {
    S = S + 1；
    if (S > 0)
        进程继续执行
    else
        释放队列中第一个等待信号量的进程
    
}
```


利用信号实现互斥：

初始化信号量mutex = 1; 当进程进入临界区时执行P操作，退出临界区时执行V操作。

```
S = 1
P(S)
    临界区
V(S)
```


利用信号量实现同步：
case 一
进程A执行，A等待进程B完成某些事情，如果B没完成A就睡眠等待，
如果B完成了A就醒来继续执行。

```
S = 0;

A进程                       B进程
...
P(S)睡眠等待V激活
...
...
执行   <------------        V(S)激活等待的P
                                               
```

case 二

```
S = n 
P(S)
    临界区
V(S)
```

应该还有其他case


看了一下LDD3 并发控制的手段还挺多：

- **各种锁：自旋锁、读写锁、顺序锁、RCU**
- **信号量：mutex、 down up、completion**
- **原子操作**
- **终极大招：中断屏蔽**

各有各的优缺点（宋老师书里的解释）

**自旋锁**

```
spin_lock 上锁后CPU就忙等直到解锁。
用途：
1 用于多处理器间共享数据
2 单核开抢占情况下共享数据

注意：
既然是忙等， 那占用锁的时间就要非常小， 不然使用自旋锁会降低系统性能。
自旋锁调用期间不能引起睡眠。
对于跑在单CPU上不支持抢占的系统， 自旋锁退化为空操作。

用法：
定义自旋锁：
    spinlock_t lock;
    
初始化自旋锁：
    spin_lock_init(lock);
    
获得自旋锁:
    spin_lock(lock);
    
尝试获得自旋锁:
    spin_trylock(lock);
    
释放自旋锁:
    spin_unlock(lock);

变种：
spin_lock_irq() = spin_lock() + local_irq_disable()
spin_unlock_irq() = spin_unlock() + local_irq_enable()
spin_lock_irqsave() = spin_lock() + local_irq_save()
spin_unlock_irqrestore() = spin_unlock() + local_irq_restore()
spin_lock_bh() = spin_lock() + local_bh_disable()
spin_unlock_bh() = spin_unlock() + local_bh_enable()


使用模板
//定义一个自旋锁
spinlock_t lock;
spin_lock_init(&lock);
spin_lock (&lock) ; //获取自旋锁，保护临界区
    临界区
spin_unlock (&lock) ; //解锁
```

**读写锁**

```
rwlock 读写锁是一种比自旋锁粒度更小的锁机制， 特殊的自旋锁。
用途：
允许同时读共享资源，但只能有一个写。读写不能同时。

用法：
定义读写锁:
rwlock_t rwlock;

初始化读写锁：
rwlock_init(&rwlock);

读锁定：
void read_lock(rwlock_t *lock);
void read_lock_irqsave(rwlock_t *lock, unsigned long flags);
void read_lock_irq(rwlock_t *lock);
void read_lock_bh(rwlock_t *lock);

读解锁：
void read_unlock(rwlock_t *lock);
void read_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
void read_unlock_irq(rwlock_t *lock);
void read_unlock_bh(rwlock_t *lock);

写锁定：
void write_lock(rwlock_t *lock);
void write_lock_irqsave(rwlock_t *lock, unsigned long flags);
void write_lock_irq(rwlock_t *lock);
void write_lock_bh(rwlock_t *lock);
int write_trylock(rwlock_t *lock);

写解锁：
void write_unlock(rwlock_t *lock);
void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags);
void write_unlock_irq(rwlock_t *lock);
void write_unlock_bh(rwlock_t *lock);

使用模板
rwlock_t lock; //定义
rwlock_init(&lock); // 初始化
read_lock(&lock);//读时获取锁
    临界区
read_unlock(&lock);

write_lock_irqsave(&lock, flags);//写时获取锁
    临界区
write_unlock_irqrestore(&lock, flags);
```

**顺序锁**

```
seqlock 是对读写锁的一种优化
说明：
    使用顺序锁，读执行单元绝不会被写执行单元阻塞，也就是说，读执行单元在写执行单元被
顺序锁保护的共享资源进行写操作时扔可以继续读， 而不必等待写执行单元完成操作， 写执行
单元也不需要等待所有读执行单元完成读操作才去执行写操作。
    但是写执行单元与写执行单元之间仍然是互斥的，即如果有写执行单元进行写操作， 其他
写执行单元必须自旋在那里，直到写执行单元释放顺序锁。
    如果读执行单元在读操作期间，写执行单元已经发生了些操作，那么读执行单元必须重新读
取数据，以确保得到的数据完整性。这种锁对于读写同时进行的概率比较小的情况，性能是非常
好的，而且它允许读写同时进行，因而更大地提高了并发性。
    顺序锁有一个限制，它必须要求被保护的共享资源不含有指针，因为写执行单元可能使得指
针失效，但读执行单元如果正要访问该指针，将导致 oops。

用法：
定义顺序锁：
seqlock_t seqlock;

初始化顺序锁：
seqlock_init(&seqlock);

读开始：
unsigned read_seqbegin(const seqlock_t *sl);
read_seqbegin_irqsave(lock, flags)
重读：
int read_seqretry(const seqlock_t *sl, unsigned iv);
read_seqretry_irqrestore(lock, iv, flags)

写锁定：
void write_seqlock(seqlock_t *sl);
int write_tryseqlock(seqlock_t *sl);
write_seqlock_irqsave(lock, flags)
write_seqlock_irq(lock)
write_seqlock_bh(lock)

写解锁：
void write_sequnlock(seqlock_t *sl);
write_sequnlock_irqrestore(lock, flags)
write_sequnlock_irq(lock)
write_sequnlock_bh(lock)

使用模板
write_seqlock(&seqlock);
    写操作代码块
write_sequnlock(&seqlock);

do {
seqnum = read_seqbegin(&seqlock);
    读操作代码块
} while (read_seqretry(&seqlock, seqnum));
```

**RCU**

这个内容比较多，先不讨论。

**mutex**

mutex就是值位1的信号量
char_driver.c中已经使用过了

```
使用模板
struct mutex mutex; //定义 mutex
mutex_init(&mutex); //初始化 mutex
mutex_lock(&mutex); //获取 mutex
    临界区
mutex_unlock(&mutex); //释放 mutex
```

**down up**

```
down 就是信号量里的P操作
up 就是信号量里的V操作

down up操作既可以用在同步中也可以用在互斥中

定义信号量：
struct semaphore sem;

初始化信号量：
void sema_init(struct semaphore *sem, int val);
该函数初始化信号量，并设置信号量 sem 的值为 val。尽管信号量可以被初始化为大于 1 的
值从而成为一个计数信号量，但是它通常不被这样使用。

获得信号量：
void down(struct semaphore * sem); //进入睡眠后进程不能被信号打断
int down_interruptible(struct semaphore * sem); //进入睡眠状态的进程可以被信号打断
int down_trylock(struct semaphore * sem); //尝试获得信号量

使用down_interruptible()获取信号量时，需要对返回值进行检查
    if (down_interruptible(&sem))
        return - ERESTARTSYS;

释放信号量：
void up(struct semaphore * sem);

使用模板：
1、用于互斥
struct semaphore sem;
sema_init(&sem,1);
down(&sem);
    临界区
up(&sem);

2、用于同步
struct semaphore sem;
sema_init(&sem,0);
A进程                               B进程
...                                 ...
down(&sem);//没获得信号量睡眠       ...
睡眠                                ...
睡眠                                ...
睡眠                                ...
获得了信号量执行                    up(&sem);   
...                                 ...

```

**completion**

```
用于同步，一个执行单元等待另一个执行单元执行完某事。

定义：
struct completion compl;

初始化：
init_completion(&compl);

等待完成量：
void wait_for_completion(struct completion *c);
//可中断的wait_for_completion
wait_for_completion_interruptible(struct completion *c); 
////带超时处理的wait_for_completion
unsigned long wait_for_completion_timeout(struct completion *x,unsigned long timeout); 

唤醒完成量:
void complete(struct completion *c); //只唤醒一个等待的执行单元
void complete_all(struct completion *c); //释放所有等待同一完成量的执行单元

使用模板：
struct completion compl;
init_completion(&compl);
A进程                               B进程
...                                 ...
wait_for_completion(&compl);        ...
睡眠                                ...
睡眠                                ...
睡眠                                ...
等待条件到了                        complete(&compl);   
...                                 ...

```

**原子操作**

原子操作指的是在执行过程中不会被别的代码路径所中断的操作

整型原子操作：

```
定义：
atomic_t i;

使用：

void atomic_set(atomic_t *v, int i); //设置原子变量的值为 i
atomic_t v = ATOMIC_INIT(0); //定义原子变量 v 并初始化为 0
atomic_read(atomic_t *v); //获取原子变量的值
void atomic_add(int i, atomic_t *v); //原子变量增加 i
void atomic_sub(int i, atomic_t *v); //原子变量减少 i
void atomic_inc(atomic_t *v); //原子变量增加 1
void atomic_dec(atomic_t *v); //原子变量减少 1

操作并测试：
int atomic_inc_and_test(atomic_t *v);
int atomic_dec_and_test(atomic_t *v);
int atomic_sub_and_test(int i, atomic_t *v);
//上述操作对原子变量执行自增、自减和减操作后（注意没有加）测试其是否为 0，为 0 返回
//true，否则返回 false。

操作并返回
int atomic_add_return(int i, atomic_t *v);
int atomic_sub_return(int i, atomic_t *v);
int atomic_inc_return(atomic_t *v);
int atomic_dec_return(atomic_t *v);
//上述操作对原子变量进行加/减和自增/自减操作，并返回新的值。

```

位原子操作:

```
void set_bit(nr, void *addr); //设置位
void clear_bit(nr, void *addr); //清除位
void change_bit(nr, void *addr); //改变位,对 addr 地址的第 nr 位进行反置。
test_bit(nr, void *addr); //测试位， 返回 addr 地址的第 nr 位。

测试并操作位
int test_and_set_bit(nr, void *addr);
int test_and_clear_bit(nr, void *addr);
int test_and_change_bit(nr, void *addr);
//上述 test_and_xxx_bit(nr, void *addr)操作等同于执行
//test_bit(nr, void *addr)后再执行xxx_bit(nr, void *addr)。
```

**中断屏蔽**
关本地中断

```
local_irq_disable()
```

开本地中断

```
local_irq_enable()
```

中断是调度的根源， 关闭中断也就不存在抢夺资源的问题了。

代码
https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver5.c

Makefile

```
obj-m := char_driver1.o \
	char_driver2.o \
	char_driver3.o \
	char_driver4.o \
	char_driver5.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
ccflags-y += -DDEBUG_MUTEX
PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

.PYTHON:clean

```

char_driver5.c 添加spin_lock、down up和completion 用于同步、原子操作



应用测试程序char5_test.c

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


char5_test.c

```
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
```

---

测试

加载设备驱动，创建设备结点。

```
# insmod char_driver4.ko

# mknod /dev/x c 222 0

```


1、测试spin_lock

```
#define DEBUG_SPIN_LOCK
//#define DEBUG_MUTEX
//#define DEBUG_SEMA
//#define DEBUG_COMPL
```
编译驱动， 插入模块， 创建文件结点

代码中spin_lock保护了打开文件的计数。

打开一个终端运行
```
# ./char5_test -o
open success

```
这时候打开文件结点后sleep 10s 才关闭文件结点

在打开一个终端， 运行失败

```
# ./char5_test -o
open fail
```
因为内核中锁上了。


2、测试down up 用于互斥

```
//#define DEBUG_SPIN_LOCK
#define DEBUG_MUTEX
//#define DEBUG_SEMA
//#define DEBUG_COMPL
```
编译驱动， 插入模块， 创建文件结点

down up用于互斥， 跟mutex的情况应该一样

```
# echo "sleep" > /dev/x
```
没有返回在等待 在开一个终端

```
# cat /dev/x
```
也没有返回，也在等待， 过了一会cat返回了

```
sleep
```
测试结果和预期一样

3、测试down up 用于同步
```
Makefile打开DDEBUG_SEMA宏
//#define DEBUG_SPIN_LOCK
#define DEBUG_MUTEX
#define DEBUG_SEMA
//#define DEBUG_COMPL
```
编译驱动， 插入模块， 创建文件结点

如果没有向设备写入， 读取就会阻塞， 如果写入了设备， 才可以读取数据。

使用test4测试

```
# ./char5_test -r 10
```
从设备读取字符， 卡住了


再开一个终端

```
# ./char5_test -w hello
```
写入字符hello

此时读那个终端也返回字符串

```
hello
```
测试结果和预期一样

4、测试completion
```
Makefile打开DDEBUG_COMPL宏
//#define DEBUG_SPIN_LOCK
#define DEBUG_MUTEX
//#define DEBUG_SEMA
#define DEBUG_COMPL
```
编译驱动， 插入模块， 创建文件结点

测试过程以及结果和 ++3、测试down up 用于同步++ 一样

值得注意的是 ++4、测试completion++ 读阻塞只能写入 才能继续， 信号无法中断( CTRL+C )

而++3、测试down up 用于同步++ 读阻塞则可以被信号中断( CTRL+C )
