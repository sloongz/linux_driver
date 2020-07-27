
- **kthread**

Linux 内核中也可以创建线程， 但与用户线程不一样 只能运行在内核空间（大于3G地址空间）， 

自己没有独立的地址空间。

几个简单的API就可以启用kthread:

```
kthread_run(threadfn, data, namefmt, ...)  //创建并唤醒线程

kthread_create(threadfn, data, namefmt, arg...) //创建线程
int wake_up_process(struct task_struct *p) //唤醒创建的线程

bool kthread_should_stop(void); //在func中判断先前是否调用过kthread_stop
int kthread_stop(struct task_struct *k) //停止线程
```

Linux进程优先级从 0~139：
- 0~99 是实时进程
- 100~139是普通进程

调度策略有三种：
- SCHED_OTHER 分时调度策略，（默认的）普通进程
- SCHED_FIFO实时调度策略，先到先服务
- SCHED_RR实时调度策略，时间片轮转 

实时进程优先调度，实时进程运行完毕才轮到普通进程调度，为了防止普通进程饿死设置

了一个门限：

```
$ cat /proc/sys/kernel/sched_rt_runtime_us
950000
$ cat /proc/sys/kernel/sched_rt_period_us
1000000
```
实时进程最多运行0.95秒，普通进程可以分配0.5秒， 

可以改变属性sched_rt_runtime_us的值， 来修改这两种进程调度的比例。


SCHED_OTHER 的进程优先级看nice值(-20~19)， 越小优先级越高。

SCHED_FIFO 进程优先级rt_priority(1-99)，不同优先级按照优先级高的先跑到睡

眠，优先级低的再跑；同等优先级先进先出， 99最大。

SCHED_RR 进程优先级rt_priority(1-99)，不同优先级按照优先级高的先跑到睡眠，

优先级低的再跑；同等优先级轮转。


写一个kthread的测试程序



编译 运行

```
插入模块
# insmod kthread.ko

查看进程号
$ ps aux | grep kthread
root          2  0.0  0.0      0     0 ?        S    02:44   0:00 [kthreadd]
root       6003  0.0  0.0      0     0 ?        D    02:52   0:00 [kthread1/1]
root       6004  0.0  0.0      0     0 ?        D    02:52   0:00 [kthread2]
root       6005  0.0  0.0      0     0 ?        D    02:52   0:00 [kthread3]
root       6006  0.0  0.0      0     0 ?        D    02:52   0:00 [kthread4]


查看创建的实时线程优先级
$ chrt -p 6003
pid 6003's current scheduling policy: SCHED_OTHER
pid 6003's current scheduling priority: 0
$ chrt -p 6004
pid 6004's current scheduling policy: SCHED_FIFO
pid 6004's current scheduling priority: 1
$ chrt -p 6005
pid 6005's current scheduling policy: SCHED_RR
pid 6005's current scheduling priority: 10
$ chrt -p 6006
pid 6006's current scheduling policy: SCHED_RR
pid 6006's current scheduling priority: 10

查看创建的普通线程优先级
$ ps -lp 6003
F S   UID    PID   PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD
1 D     0   6003      2  0  80   0 -     0 -      ?        00:00:00 kthread1/1
看NI那列nice值为0

卸载模块
# rmmod kthread.ko

$ dmesg
[  447.930438] kthread4 cnt4:0
[  447.930444] kthread3 cnt3:0
[  447.930447] kthread2 cnt2:0
[  447.930450] kthread1 cnt1:0
[  448.954609] kthread3 cnt3:1
[  448.954617] kthread4 cnt4:1
[  448.954621] kthread2 cnt2:1
[  448.954627] kthread1 cnt1:1
[  449.978576] kthread4 cnt4:2
[  449.978582] kthread3 cnt3:2
[  449.978586] kthread2 cnt2:2
[  449.978589] kthread1 cnt1:2
[  451.001721] kthread3 cnt3:3
[  451.001730] kthread4 cnt4:3
[  451.001734] kthread2 cnt2:3
[  451.001739] kthread1 cnt1:3
[  452.025596] kthread4 cnt4:4
[  452.025604] kthread3 cnt3:4
[  452.025607] kthread2 cnt2:4
[  452.025610] kthread1 cnt1:4
[  453.050032] kthread3 cnt3:5
[  453.050042] kthread4 cnt4:5
[  453.050046] kthread2 cnt2:5
[  453.050052] kthread1 cnt1:5
[  454.073823] kthread4 cnt4:6
[  454.073833] kthread3 cnt3:6
[  454.073837] kthread2 cnt2:6
[  454.073842] kthread1 cnt1:6
[  455.097701] kthread3 cnt3:7
[  455.097709] kthread4 cnt4:7
[  455.097713] kthread2 cnt2:7
[  455.097717] kthread1 cnt1:7
[  456.121685] kthread4 cnt4:8
[  456.121689] kthread3 cnt3:8
[  456.121691] kthread2 cnt2:8
[  456.121693] kthread1 cnt1:8
[  457.145794] kthread3 cnt3:9
[  457.145803] kthread4 cnt4:9
[  457.145806] kthread2 cnt2:9
[  457.145811] kthread1 cnt1:9
...
[  518.574982] x_exit end
```
看到创建的线程运行起来了， 而且是实时的先调度， 退出正常。

