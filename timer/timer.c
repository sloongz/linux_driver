/*
 * a timer kernel module: timer
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h> // 定时器
#include <linux/time.h> //current_kernel_time()需要这个宏

#include <linux/version.h>

static struct timer_list g_timer;
static struct timespec g_ts;
 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
static void timer_func(struct timer_list *t)
#else
static void timer_func(unsigned long data)
#endif
{
	g_ts = current_kernel_time();
	printk(KERN_INFO "%s jiffies:%ld\n", __func__, jiffies);
	printk("current time:%lds.%ldns \n", g_ts.tv_sec, g_ts.tv_nsec);
	//timer一旦超时，就会执行func函数，然后永远的休眠
	//mod_timer可以激活timer
	mod_timer(&g_timer, jiffies + 1*HZ);
}

int timer_init(void) 
{
	g_ts = current_kernel_time();//此函数用于返回当前内核时间。该时间是距离1970开始的秒和纳秒
	printk("1 sec is %lu jiffies\n", msecs_to_jiffies(1000));
	printk(KERN_INFO "%s jiffies:%ld\n", __func__, jiffies);
	printk("current time:%lds.%ldns \n", g_ts.tv_sec, g_ts.tv_nsec);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
	timer_setup(&g_timer, timer_func, 0);//4.15版本修改了接口
#else
	init_timer(&g_timer);
	g_timer.function = &timer_func;
#endif

	g_timer.expires = jiffies + 5*HZ; //超时5秒，执行function
	add_timer(&g_timer);

	return 0;
}

void timer_exit(void) 
{
	del_timer(&g_timer);//删除timer
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a timer module");
MODULE_ALIAS("a timer module");
