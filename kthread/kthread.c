/*
 * a kthread module: kthread
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/sched.h>
//#include <uapi/linux/sched/types.h>
#include <linux/kthread.h>
#include <linux/err.h>

struct x_dev {
	struct task_struct *kthread1;
	struct task_struct *kthread2;
	struct task_struct *kthread3;
	struct task_struct *kthread4;
	int cnt1;
	int cnt2;
	int cnt3;
	int cnt4;
};

struct x_dev g_dev_priv;

static int kthread1_func(void *args)
{
	struct x_dev *dev_priv = (struct x_dev *)args;

	while (1) {
		if( kthread_should_stop())  
			return -1; 
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1*HZ);
		printk("kthread1 cnt1:%d\n", dev_priv->cnt1++);
	};

	return 0;	
}

static int kthread2_func(void *args)
{
	struct x_dev *dev_priv = (struct x_dev *)args;

	while (1) {
		if( kthread_should_stop())  ////判断先前是否调用过kthread_stop
			return -1; 
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1*HZ); //睡眠让出CPU 1S后在运行
		printk("kthread2 cnt2:%d\n", dev_priv->cnt2++);
	};

	return 0;	
}

static int kthread3_func(void *args)
{
	struct x_dev *dev_priv = (struct x_dev *)args;

	while (1) {
		if( kthread_should_stop())  ////判断先前是否调用过kthread_stop
			return -1; 
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1*HZ); //睡眠让出CPU 1S后在运行
		printk("kthread3 cnt3:%d\n", dev_priv->cnt3++);
	};

	return 0;	
}

static int kthread4_func(void *args)
{
	struct x_dev *dev_priv = (struct x_dev *)args;

	while (1) {
		if( kthread_should_stop())  ////判断先前是否调用过kthread_stop
			return -1; 
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1*HZ); //睡眠让出CPU 1S后在运行
		printk("kthread4 cnt4:%d\n", dev_priv->cnt4++);
	};

	return 0;	
}


static int __init x_init(void)
{
	struct sched_param param;
	int val = 1;

	g_dev_priv.cnt1 = 0;
	g_dev_priv.cnt2 = 0;
	g_dev_priv.cnt3 = 0;
	g_dev_priv.cnt4 = 0;

	/*#define kthread_run(threadfn, data, namefmt, ...)              \
	({                                     \
		struct task_struct *__k                        \
			= kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
		if (!IS_ERR(__k))                          \
			wake_up_process(__k);                      \
		__k;                                   \
	})
	*/
	g_dev_priv.kthread1 = kthread_run(&kthread1_func, &g_dev_priv, "kthread1/%d", val);

	g_dev_priv.kthread2 = kthread_create(&kthread2_func, &g_dev_priv, "kthread2");
	if (IS_ERR(g_dev_priv.kthread2)) {
		printk("create kthread fail\n");		
		return -1;	
	}
	param.sched_priority = 1; //设置进程优先级
	sched_setscheduler(g_dev_priv.kthread2, SCHED_FIFO /*SCHED_RR*/, &param);


	g_dev_priv.kthread3 = kthread_create(&kthread3_func, &g_dev_priv, "kthread3");
	if (IS_ERR(g_dev_priv.kthread3)) {
		printk("create kthread fail\n");		
		return -1;	
	}
	param.sched_priority = 10; //设置进程优先级
	sched_setscheduler(g_dev_priv.kthread3, /*SCHED_FIFO*/ SCHED_RR, &param);


	g_dev_priv.kthread4 = kthread_create(&kthread4_func, &g_dev_priv, "kthread4");
	if (IS_ERR(g_dev_priv.kthread4)) {
		printk("create kthread fail\n");		
		return -1;	
	}
	param.sched_priority = 10; //设置进程优先级
	sched_setscheduler(g_dev_priv.kthread4, /*SCHED_FIFO*/ SCHED_RR, &param);


	wake_up_process(g_dev_priv.kthread2);
	wake_up_process(g_dev_priv.kthread3);
	wake_up_process(g_dev_priv.kthread4);

	return 0;
}

static void __exit x_exit(void)
{
	kthread_stop(g_dev_priv.kthread1);
	kthread_stop(g_dev_priv.kthread2);
	kthread_stop(g_dev_priv.kthread3);
	kthread_stop(g_dev_priv.kthread4);
	printk("%s end\n", __func__);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a kthread module");
MODULE_ALIAS("a kthread module");
