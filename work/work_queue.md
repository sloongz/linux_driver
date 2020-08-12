
- **work_queue**

工作队列是使用最多的延迟执行机制， 它可以把工作交由一个内核线程去执行（进程上下文）。

这样工作队列就允许重新调度甚至是睡眠。

接口：

创建任务
```
INIT_WORK(struct work_struct work, work_func_t func);
INIT_DELAYED_WORK(struct delayed_work work, work_func_t func); 
```
调度任务：

```
int schedule_work(struct work_struct *work); 
int schedule_delayed_work(struct delayed_work *work, unsigned long delay);
```

删除任务：

```
int cancel_work_sync(struct work_struct *work));
int cancel_delayed_work_sync(struct work_struct *work));
```


以上方法是添加到系统的共享工作队列， 如果想自己创建工作队列则用下面接口：

创建工作队列：
```
create_singlethread_workqueue(name) 
create_workqueue(name)
```
create_singlethread_workqueue是在一个核上创建工作队列

create_workqueue是在每个核上都创建工作队列

创建任务：

```
上面提到过
```

向工作队列提交任务（相当于schedule_work和schedule_delayed_work）：

```
int queue_work(workqueue_t *queue, work_t *work); 
int queue_delayed_work(workqueue_t *queue, work_t *work, unsigned long delay);
```
释放工作队列：

```
void destroy_workqueue(struct workqueue_struct *queue);
```

代码

https://github.com/sloongz/linux_driver/tree/master/work

测试：

```
# insmod work_queue.ko 
# rmmod work_queue
```
看log

```
[11334.053576] init work
[11334.053594] work_func, a:0
[11334.053603] dly_work_func, b:0
[11334.054429] dly_work1_func, d:0
[11334.054432] work1_func, c:0
[11335.084279] dly_work_func, b:1
[11335.084284] dly_work1_func, d:1
[11335.084286] work_func, a:1
[11335.084293] work1_func, c:1
[11336.108317] dly_work_func, b:2
[11336.108322] dly_work1_func, d:2
[11336.108324] work_func, a:2
[11336.108331] work1_func, c:2
[11337.132295] dly_work_func, b:3
[11337.132300] dly_work1_func, d:3
[11337.132302] work_func, a:3
[11337.132309] work1_func, c:3
[11338.156346] dly_work_func, b:4
[11338.156351] dly_work1_func, d:4
[11338.156353] work_func, a:4
[11338.156360] work1_func, c:4
[11339.180426] dly_work_func, b:5
[11339.180431] dly_work1_func, d:5
[11339.180433] work_func, a:5
[11339.180439] work1_func, c:5
[11340.204479] dly_work_func, b:6
[11340.204484] dly_work1_func, d:6
[11340.204486] work_func, a:6
[11340.204493] work1_func, c:6
[11341.227451] dly_work_func, b:7
[11341.227453] dly_work1_func, d:7
[11341.227453] work_func, a:7
[11341.227456] work1_func, c:7
[11342.251582] dly_work_func, b:8
[11342.251586] dly_work1_func, d:8
[11342.251589] work_func, a:8
[11342.251595] work1_func, c:8
[11343.275642] dly_work_func, b:9
[11343.275646] dly_work1_func, d:9
[11343.275648] work_func, a:9
[11343.275654] work1_func, c:9
[11344.299572] dly_work_func, b:10
[11344.299575] dly_work1_func, d:10
[11344.299576] work_func, a:10
[11344.299579] work1_func, c:10
[11345.323773] dly_work_func, b:11
[11345.323778] dly_work1_func, d:11
[11345.323780] work1_func, c:11
[11346.347734] dly_work1_func, d:12
[11346.347922] cancel work
```
可以看到创建的四个任务几乎同时运行， 也可以移除。