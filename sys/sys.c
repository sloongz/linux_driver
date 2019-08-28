/*
 * a sys kernel module: sys
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/cdev.h>

#define MEM_SIZE 1024

static struct kobject *g_kobj = NULL;
static char *g_str;
static unsigned char *g_bin;

static ssize_t show_test(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", g_str);
}

static ssize_t store_test(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count) 
{
	memset(g_str, 0, MEM_SIZE);
	if (count > MEM_SIZE)
	  count = MEM_SIZE;

	memcpy(g_str, buf, count);	
	return count;	
}

//static struct kobj_attribute dev_attr_xxx =
//__ATTR(xxx, 0664, (void *)show_test, (void *)store_test);
static DEVICE_ATTR(xxx, 0664, show_test, store_test);
//文本属性文件
static struct attribute *dev_attr[] = {
	&dev_attr_xxx.attr,
	NULL
};

static ssize_t bbb_show(struct file *fp,
			        struct kobject *kobj, struct bin_attribute *attr, char *buf,
					        loff_t pos, size_t count)
{
	ssize_t ret;

	ret = memory_read_from_buffer(buf, count, &pos, g_bin, MEM_SIZE);

	return ret;
}

static ssize_t bbb_store(struct file *filp, struct kobject *kobj,
			struct bin_attribute *bin_attr,
			char *buf, loff_t pos, size_t count)
{
	if ((pos + count) > MEM_SIZE)	
		return -EINVAL;

	memcpy(g_bin + pos, buf, count);

	return count;
}

//struct bin_attribute bin_attr_bbb = { 
//	.attr =  {
//		.name = __stringify(bbb),
//		.mode = 0600,
//		},
//	.read =  bbb_show,
//	.write = bbb_store,
//	.size = MEM_SIZE,
//};

static BIN_ATTR(bbb, 0644, bbb_show, bbb_store, MEM_SIZE);
//二进制属性文件
static struct bin_attribute *dev_bin_attr[] = {
	&bin_attr_bbb,	
	NULL
};

//创建两个属性文件
static const struct attribute_group dev_attr_grp = {
	.attrs = dev_attr,
	.bin_attrs = dev_bin_attr,
};


static int __init x_init(void)
{
	g_str = kmalloc(MEM_SIZE, GFP_KERNEL);
	if (!g_str) {
		printk("alloc mem fail\n");
	}
	memset(g_str, 0, MEM_SIZE);

	g_bin = kmalloc(MEM_SIZE, GFP_KERNEL);
	if (!g_bin) {
		printk("alloc mem fail\n");
		goto mem_err;
	}
	memset(g_bin, 0, MEM_SIZE);

	if ((g_kobj = kobject_create_and_add("test_sysfs", NULL)) == NULL ) {
		printk(KERN_ERR "create sys fail\n");
		goto mem_err1;
	} else {
		if (sysfs_create_group(g_kobj, &dev_attr_grp)) {
			printk(KERN_ERR "create sys group fail\n");
			goto err2;
		}
	}

	return 0;
err2:
	kobject_put(g_kobj);
mem_err1:
	kfree(g_bin);
mem_err:
	kfree(g_str);
	return -1;
}

static void __exit x_exit(void)
{
	if (g_kobj) {
		sysfs_remove_group(g_kobj, &dev_attr_grp);	
		kobject_put(g_kobj);
	}

	kfree(g_str);
	kfree(g_bin);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a sys module");
MODULE_ALIAS("a sys test module");
