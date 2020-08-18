tasklet 是软中断的一种， 不可以睡眠， 可以被中断打断， 但是不能被其他软中断和进程打断。

类型：

```
const char * const softirq_to_name[NR_SOFTIRQS] = {
    "HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL",
    "TASKLET", "SCHED", "HRTIMER", "RCU"
};
```
HI：用于高优先级的tasklet

TIMER：用于定时器的下半部

NET_TX：用于网络层发包

NET_RX：用于网络层收报

TASKLET：用于低优先级的tasklet

使用API：

```
//动态初始化tasklet
void tasklet_init(struct tasklet_struct *t,void (*func)(unsigned long), unsigned long data)

//静态初始化tasklet
DECLART_TASKLET(name,func,data)

//禁止给定的tasklet被tasklet_schedule调度，如果该tasklet当前正在执行，这个函数会等到它执行完毕再返回。
static inline void tasklet_disable(struct tasklet_struct *t)

//不等待就返回
void tasklet_disable_nosync(struct tasklet_struct *t)。

//disable 后可以调用enable使能
static inline void tasklet_enable(struct tasklet_struct *t)

//调度
static inline void tasklet_schedule(struct tasklet_struct *t)

//高优先级调度
tasklet_hi_schedule(struct tasklet_struct *t)

//清除，如果tasklet 正在运行，函数会一直等知道执行完毕。
tasklet_kill(struct tasklet_struct *t)
```

代码：

https://github.com/sloongz/linux_driver/blob/master/tasklet/tasklet.c

使用一个任务触发tasklet, 5s后disable tasklet 10s后在enable


```
$ sudo insmod tasklet.ko
```
过20秒再

```
$ sudo rmmod tasklet
```
查看log
```
$ dmesg
[  137.629846] x_init enter
[  137.629859] delay work, cnt:0
[  137.629868] tasklet_handler, data:my tasklet
[  138.639885] delay work, cnt:1
[  138.639898] tasklet_handler, data:my tasklet
[  139.663493] delay work, cnt:2
[  139.663505] tasklet_handler, data:my tasklet
[  140.687576] delay work, cnt:3
[  140.687859] tasklet_handler, data:my tasklet
[  141.711785] delay work, cnt:4
[  142.735132] delay work, cnt:5
[  143.758957] delay work, cnt:6
[  144.782863] delay work, cnt:7
[  145.806788] delay work, cnt:8
[  146.830743] delay work, cnt:9
[  146.830747] tasklet_handler, data:my tasklet
[  147.855063] delay work, cnt:10
[  147.855075] tasklet_handler, data:my tasklet
[  148.879044] delay work, cnt:11
[  148.879056] tasklet_handler, data:my tasklet
[  149.902629] delay work, cnt:12
[  149.902643] tasklet_handler, data:my tasklet
[  150.926725] delay work, cnt:13
[  150.926739] tasklet_handler, data:my tasklet
[  151.950538] delay work, cnt:14
[  151.950551] tasklet_handler, data:my tasklet
[  152.974503] delay work, cnt:15
[  152.974517] tasklet_handler, data:my tasklet
[  153.998330] delay work, cnt:16
[  153.998342] tasklet_handler, data:my tasklet
[  155.022088] delay work, cnt:17
[  155.022097] tasklet_handler, data:my tasklet
[  156.046054] delay work, cnt:18
[  156.046065] tasklet_handler, data:my tasklet
[  157.070120] delay work, cnt:19
[  157.070133] tasklet_handler, data:my tasklet
[  158.093845] delay work, cnt:20
[  158.093855] tasklet_handler, data:my tasklet
[  159.116922] delay work, cnt:21
[  159.116934] tasklet_handler, data:my tasklet
[  160.140769] delay work, cnt:22
[  160.140782] tasklet_handler, data:my tasklet
[  161.165575] delay work, cnt:23
[  161.165588] tasklet_handler, data:my tasklet
[  161.591033] x_exit exit

```

