/*
 * a simple kernel module: fs
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>

static int __init simplefs_init(void)
{
	printk(KERN_INFO "hello world enter\n");
	return 0;
}


static void __exit simplefs_exit(void)
{
	printk(KERN_INFO "hello world exit\n");
}

module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
