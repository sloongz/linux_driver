/*
 * a proc test kernel module: proc
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define MEM_SIZE 1024 

static char *g_str;
static struct proc_dir_entry *test_dir, *test_entry;

//read的时候会调到
static int test_proc_show(struct seq_file *seq, void *v)
{
	char *buf = seq->private;

	seq_printf(seq, "%s\n", buf);
	return 0;
}

static int test_proc_open(struct inode *inode, struct file *file)
{
	//使用PDE_DATA(inode)作为私有数据向下传
	return single_open(file, test_proc_show, PDE_DATA(inode));	
}

static ssize_t test_proc_write(struct file *file,
			const char __user *buffer,
			size_t count, loff_t *pos)
{
	struct seq_file *seq = file->private_data;
	char *buf = seq->private;
	int ret;

	if (!buffer || count > MEM_SIZE - 1)
		return -EINVAL;	
	
	memset(buf, 0, MEM_SIZE);
	ret = copy_from_user(buf, buffer, count);
	
	return count - ret;
}

static const struct file_operations test_proc_fops = {
	.owner      = THIS_MODULE,
	.open       = test_proc_open, //需要自己写
	.read       = seq_read,	//不用管
	.llseek     = seq_lseek, //不用管
	.release    = single_release, //不用管
	.write      = test_proc_write, //需要自己写
};

static int __init test_proc_init(void)
{
	g_str = kmalloc(MEM_SIZE, GFP_KERNEL);
	if (!g_str) {
		printk("alloc mem fail\n");
	}
	memset(g_str, 0, MEM_SIZE);

	//创建目录
	test_dir = proc_mkdir("test_dir", //目录名
					NULL); //父母目录， 如果是NULL， 父目录为/proc

	if (test_dir) {
		//创建属性文件
		test_entry = proc_create_data("test_rw", //属性名字
						0666, //读写权限
						test_dir, //父目录
						&test_proc_fops, //操作函数表
						g_str);//私有数据
		if (test_entry == NULL) {
			return -ENOMEM;
		}
	}

	return 0;
}

static void __exit test_proc_exit(void)
{
	remove_proc_entry("test_rw", test_dir);//删除属性
	remove_proc_entry("test_dir", NULL);//删除目录
	kfree(g_str);
}

module_init(test_proc_init);
module_exit(test_proc_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a proc test module");
MODULE_ALIAS("a proc test module");
