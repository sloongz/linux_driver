/*
 * a debugfs test kernel module: debugfs
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define MEM_SIZE 1024 

static char *g_str;
struct dentry *g_dir;

//read的时候会调到
static int test_debugfs_show(struct seq_file *seq, void *v)
{
	char *buf = seq->private;

	seq_printf(seq, "%s\n", buf);
	return 0;
}

static int test_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, test_debugfs_show, inode->i_private);	
}

static ssize_t test_debugfs_write(struct file *file,
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

static const struct file_operations test_debugfs_fops = {
	.owner      = THIS_MODULE,
	.open       = test_debugfs_open, //需要自己写
	.read       = seq_read,	//不用管
	.llseek     = seq_lseek, //不用管
	.release    = single_release, //不用管
	.write      = test_debugfs_write, //需要自己写
};

static int __init x_init(void)
{
	g_str = kmalloc(MEM_SIZE, GFP_KERNEL);
	if (!g_str) {
		printk("alloc mem fail\n");
	}
	memset(g_str, 0, MEM_SIZE);
//static inline struct dentry *debugfs_create_dir(const char *name, struct dentry *parent)
	g_dir = debugfs_create_dir("test_debugfs",NULL);
	if (!g_dir) {
		printk(KERN_ERR "create debugfs dir fail\n");
		goto err;	
	} else {
// 50 struct dentry *debugfs_create_file(const char *name, umode_t mode,
//                    struct dentry *parent, void *data,
//                    const struct file_operations *fops);
		if (!debugfs_create_file("test_rw", 0644, g_dir, g_str, &test_debugfs_fops))
		  goto err1;
	}

	return 0;
err:
	kfree(g_str);
err1:
	debugfs_remove_recursive(g_dir);
	return -1;
}

static void __exit x_exit(void)
{
	debugfs_remove_recursive(g_dir);
	kfree(g_str);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a debugfs test module");
MODULE_ALIAS("a debugfs test module");
