
**阻塞**

- **等待队列**


等待队列用于阻塞，异步通知：

阻塞操作是指在执行设备操作时，若不能获得资源，则挂起进程直到满足可操作的条件后再

进行操作。被挂起的进程进入休眠状态，被从调度器的运行队列移走，直到等待的条件被满足。

而非阻塞操作的进程在不能进行设备操作时，并不挂起，它或者放弃，或者不停地查询，直至可

以进行操作为止。


进程状态：
![进程状态](https://github.com/sloongz/Tools/blob/master/image/char_driver6.png?raw=true)


```
就绪 TASK_RUNNING：
    进程需要的资源已经就位，进程处于可运行状态，但进程未占用CPU在待调度到运行状；
运行： 
    占有CPU， 正在运行线程，此时的标志也是TASK_RUNNING；
深度睡眠 TASK_UNINTERRUPTIBLE：
    等待的资源到位后才醒来，不能由外部信号唤醒，只能由内核自己唤醒；
浅度睡眠 TASK_INTERRUPTIBLE:
    等待的资源到位或收到信号后都会醒来；
暂停 TASK_STOPPED：
    stop 状态是被外部命令作业控制等强制进程进入的状态。
僵死 TASK_ZOMBLE：
    子进程退出后，所有资源都消失了，只剩下 task_struct，父进程在 wait 
    函数中可以得到子进程的死亡原因。在 wait 之前子进程的状态就是僵尸态。
```

等待队列就是让进程进入TASK_UNINTERRUPTIBLE状态或者TASK_INTERRUPTIBLE， 等待资源或

信号到位才唤醒。

使用：

```
定义：
wait_queue_head_t queue;

初始化：
init_waitqueue_head(&queue);

等待事件
//queue等待队列头，condition条件
void wait_event(wait_queue_head_t queue, int condition)
//可被信号打断
int wait_event_interruptible(wait_queue_head_t queue, int condition);
//timeout到达时，无论condition是否满足，均返回
int wait_event_timeout(wait_queue_head_t queue, int condition, int time);
int wait_event_interruptible_timeout(wait_queue_head_t queue, int condition,int time);


唤醒队列
void wake_up(wait_queue_head_t *queue);
void wake_up_interruptible(wait_queue_head_t *queue);

```
成对使用
```

wake_up()                       wait_event()
                                wait_event_timeout()
                
wake_up_interruptible()         wait_event_interruptible()
                                wait_event_interruptible_timeout()
```

在char_driver4.c的基础上新建char_driver6.c, 添加以下功能 
- 如果设备缓冲区没有数据则读阻塞，直到有数据填入
- 如果设备缓冲区已经满了则写阻塞，直到有数据被读出


---
测试

编译，插入模块，创建设备结点

使用之前写好的测试程序。

1 测试读阻塞

设备刚创建时，缓冲区是空的

```
# ./char5_test -r 10

```
读操作阻塞

再开一个终端写入数据

```
# echo "hello" > /dev/x
```
这时读操作返回

```
hello
```

2 测试写操作

```
# ./char3_test
usage: char3_test <x>
x: w - write dev buf
   r - read dev buf
   s - set dev buf 's'
   c - clear dev buf

# ./char3_test s
```
将设备缓冲区写满字符's'

然后写设备

```
# echo "hello" > /dev/x
```
卡住了，没办法写入

读出几个字符串

```
# ./char5_test -r 10
ssssssssss
```
在去写设备就可以写了

```
# echo "hello" > /dev/x
```
正常返回

读取设备得缓存

```
# ./char5_test -r 1024
hello
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss

```
hello已经写入了

测试1和测试2的结果和预期一样，读写都可以阻塞，条件满足了就继续运行。


值得注意的是测试结果和char_driver5.c的completion一样。

看一下include/linux/completion.h的定义

```
/*
 * struct completion - structure used to maintain state for a "completion"
 *
 * This is the opaque structure used to maintain the state for a "completion".
 * Completions currently use a FIFO to queue threads that have to wait for
 * the "completion" event.
 *
 * See also:  complete(), wait_for_completion() (and friends _timeout,
 * _interruptible, _interruptible_timeout, and _killable), init_completion(),
 * reinit_completion(), and macros DECLARE_COMPLETION(),
 * DECLARE_COMPLETION_ONSTACK().
 */
struct completion {
    unsigned int done;
    wait_queue_head_t wait;
};

```
completion也是封装了等待队列，难怪功能都差不多。