/*
 * a work_queue kernel module: work_queue
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

struct x_dev {
	struct delayed_work dly_work;
	int a;
	struct work_struct work;
	int b;
	struct work_struct work1;
	int c;
	struct delayed_work dly_work1;
	int d;
	struct workqueue_struct *workq;
}; 

static struct x_dev g_dev;

//方法一
static void work_func(struct work_struct *work)
{
	struct x_dev *devp = container_of(work, struct x_dev, work);

	printk("%s, a:%d\n", __func__, devp->a++);	
	msleep(1*1000);	
	schedule_work(&g_dev.work);//调度任务
}

//方法二
static void dly_work_func(struct work_struct *work)
{
	struct x_dev *devp = container_of(work, struct x_dev, dly_work.work);

	printk("%s, b:%d\n", __func__, devp->b++);	
	schedule_delayed_work(&devp->dly_work, msecs_to_jiffies(1000));//等待1s后调度任务
}

//方法三
static void work1_func(struct work_struct *work)
{
	struct x_dev *devp = container_of(work, struct x_dev, work1);

	printk("%s, c:%d\n", __func__, devp->c++);	
	msleep(1*1000);	
	queue_work(devp->workq, &devp->work1);//调度任务
}

//方法四
static void dly_work1_func(struct work_struct *work)
{
	struct x_dev *devp = container_of(work, struct x_dev, dly_work1.work);

	printk("%s, d:%d\n", __func__, devp->d++);
	//等待1s后调度任务
	queue_delayed_work(devp->workq, &devp->dly_work1, msecs_to_jiffies(1000));
}

static int __init x_init(void)
{
	g_dev.a = 0;
	g_dev.b = 0;
	g_dev.c = 0;
	g_dev.d = 0;

	printk("init work\n");
	//方法一
	INIT_WORK(&g_dev.work, work_func);//动态初始化一个任务
	schedule_work(&g_dev.work);//调度任务

	//方法二
	INIT_DELAYED_WORK(&g_dev.dly_work, dly_work_func);//动态初始化一个任务
	//schedule_delayed_work(&g_dev.dly_work, msecs_to_jiffies(1000));
	schedule_work(&g_dev.dly_work.work);//调度任务

	//g_dev.workq = create_singlethread_workqueue("test");
	g_dev.workq = create_workqueue("test");//创建新的工作队列
	if (!g_dev.workq) {
		printk("create workqueue fail\n");	
	} else {
		//将创建的任务挂在“test”工作队列
		//方法四
		INIT_DELAYED_WORK(&g_dev.dly_work1, dly_work1_func); 
		//方法三
		INIT_WORK(&g_dev.work1, work1_func);
		//方法四
		//queue_delayed_work(g_dev.workq, &g_dev.dly_work1, 0);
		queue_work(g_dev.workq, &g_dev.dly_work1.work); //调度任务
		//方法三
		queue_work(g_dev.workq, &g_dev.work1);//调度任务
	}

	return 0;
}


static void __exit x_exit(void)
{
	//删除任务
	cancel_work_sync(&g_dev.work);
	cancel_delayed_work_sync(&g_dev.dly_work);
	cancel_work_sync(&g_dev.work1);
	cancel_delayed_work_sync(&g_dev.dly_work1);
	destroy_workqueue(g_dev.workq);//注销工作队列
	printk("cancel work\n");
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a work_queue module");
MODULE_ALIAS("a work_queue module");
