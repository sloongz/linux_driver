/*
 * a simple kernel module: hello
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
	printk(KERN_INFO "hello world enter\n");
	return 0;
}


static void __exit hello_exit(void)
{
	printk(KERN_INFO "hello world exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a simple hello world module");
MODULE_ALIAS("a simple test module");
