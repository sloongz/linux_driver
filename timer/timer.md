
- **timer**

定时器类似于软中断，可以指定事件发生的时间。而且是处于非进程的上下文中，

所以调度函数必须遵守以下规则：

1) 没有 current 指针、不允许访问用户空间。因为没有进程上下文，相关代码和被中断的进程没有任何联系。

2) 不能执行休眠（或可能引起休眠的函数）和调度。

3) 任何被访问的数据结构都应该针对并发访问进行保护，以防止竞争条件。

内核定时器的数据结构：
```
struct timer_list {
    /*  
     * All fields that change during normal runtime grouped to the
     * same cacheline
     */
    struct list_head entry; //定时器列表
    unsigned long expires;  //定时器到期时间
    struct tvec_base *base; //作为参数被传入定时器处理函数

    void (*function)(unsigned long); //定时器处理函数
    unsigned long data; //作为参数被传入定时器处理函数

    int slack;

#ifdef CONFIG_TIMER_STATS
    int start_pid;
    void *start_site;
    char start_comm[16];
#endif
#ifdef CONFIG_LOCKDEP
    struct lockdep_map lockdep_map;
#endif
};

```

timer.c代码

先初始化一个timer，加入调度队列，时间到了就触发func，

在func中再更新timer的时间。

https://github.com/sloongz/linux_driver/blob/master/timer/timer.c

插入驱动， dmesg打印log

```
[ 5030.023432] 1 sec is 250 jiffies
[ 5030.023436] timer_init jiffies:4296149852
[ 5030.023438] current time:1540453666s.554037007ns 
[ 5035.031087] timer_func jiffies:4296151104
[ 5035.031121] current time:1540453671s.562246142ns 
[ 5036.055018] timer_func jiffies:4296151360
[ 5036.055022] current time:1540453672s.586288917ns 
[ 5037.079064] timer_func jiffies:4296151616
[ 5037.079099] current time:1540453673s.610331702ns 
[ 5038.102112] timer_func jiffies:4296151872
[ 5038.102148] current time:1540453674s.634374498ns
```
打印的和预期一样。