/*
 * a simple softirq kernel module: tasklet
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

static struct delayed_work dly_work;
static struct tasklet_struct mytasklet;

struct mydata {
	char name[64];	
};

static struct mydata *data;
static int cnt = 0;

static void dly_work_func(struct work_struct *work)
{
	printk(KERN_INFO "delay work, cnt:%d\n", cnt++);
	if (cnt == 5) {
		tasklet_disable(&mytasklet);	
		//tasklet_disable_nosync(&mytasklet);	
	}
	if (cnt == 10) {
		tasklet_enable(&mytasklet);
	}
	tasklet_schedule(&mytasklet);
	schedule_delayed_work(&dly_work, msecs_to_jiffies(1000));//等待1s后调度任务
}

void tasklet_handler(unsigned long data) 
{
	struct mydata *p = (void *)data;
	printk(KERN_INFO "%s, data:%s\n", __func__, p->name);
}

static int __init x_init(void)
{

	printk(KERN_INFO "x_init enter\n");

	data = kzalloc(sizeof(struct mydata), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	//DECLARE_TASKLET(name, func, data);
	strcpy(data->name, "my tasklet");

	tasklet_init(&mytasklet, tasklet_handler, (unsigned long)data);

	INIT_DELAYED_WORK(&dly_work, dly_work_func);
	schedule_work(&dly_work.work);

	return 0;
}


static void __exit x_exit(void)
{
	printk(KERN_INFO "x_exit exit\n");
	cancel_delayed_work_sync(&dly_work);
	tasklet_kill(&mytasklet);
	kfree(data);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a simple tasklet module");
MODULE_ALIAS("a simple tasklet module");
