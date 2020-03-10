/*
 * a char kernel module: char_driver
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>

//模块初始化
static int __init x_init(void)
{
	int ret;
	printk(KERN_INFO "netlink create\n");
	return 0;
}

//模块注销
static void __exit x_exit(void)
{
	printk(KERN_INFO "netlink release\n");
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a netlink module");
MODULE_ALIAS("a char netlink module");
