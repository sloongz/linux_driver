/*
 * a char kernel module: char_driver
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

//主设备号
#define X_MAJOR 222

#define G_MEM_SIZE 1024

static int g_major = X_MAJOR;

//全局缓存
static char g_buf[G_MEM_SIZE] = {0};

static int x_open(struct inode *inode, struct file *filp)
{
    //私有数据指针， 大多数指向驱动中自定义的设备结构体
	filp->private_data = g_buf;

	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static int x_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

//read write 函数
//参数：
//filp 文件指针
//buf 用户空间指针
//size 请求传输的数据长度（字节）
//f_ops 用户在文件中进行存取操作的偏移量

static ssize_t x_read(struct file *filp, char __user *buf, size_t size,
			loff_t *f_pos)
{
	int ret = 0;
	unsigned int cnt = size;
	unsigned long p = *f_pos;
	char *mem_p = filp->private_data;
	
	//操作位置到文件尾，或超出文件尾了
	if (p > G_MEM_SIZE)
	    return 0;
	  
	//在当前位置所要读的数目超过文件尾了
	if (cnt > G_MEM_SIZE - p)
		cnt = G_MEM_SIZE - p;

	if (copy_to_user(buf, mem_p + p, cnt)) {
		ret = -EFAULT;	
	} else {
		*f_pos += cnt;
		ret = cnt;
	}

	return ret;
}

static ssize_t x_write(struct file *filp, const char __user *buf,
			size_t size, loff_t *f_pos)
{
	int ret = 0;
	unsigned int cnt = size;
	unsigned long p = *f_pos;
	char *mem_p = filp->private_data;

	if (p > G_MEM_SIZE)
        return 0;
	if (cnt > G_MEM_SIZE - p)
		cnt = G_MEM_SIZE - p;

	if (copy_from_user(mem_p + p, buf, cnt)) {
		ret = -EFAULT;
	} else {
		*f_pos += cnt;
		ret = cnt;
	}

	return ret;
}

//填充file_operations结构体
static struct file_operations x_fops = {  
	.owner =    THIS_MODULE,  
	.open =  x_open,  
	.release =  x_release,  
	.read =     x_read,  
	.write =    x_write,  
}; 

//模块初始化
static int __init x_init(void)
{
	int ret;

	printk(KERN_INFO "char enter\n");

    //使用老方法注册字符设备， 名字char_driver，主设备号g_major的值，绑定文件操作接口x_fops
	ret = register_chrdev(g_major, "char_driver", &x_fops);	
	if (ret < 0) {
		printk(KERN_ERR "unable to register char dev %d\n", g_major);
		return ret;
	}

	return 0;
}

//模块注销
static void __exit x_exit(void)
{
	printk(KERN_INFO "char exit\n");
	//注销字符设备
	unregister_chrdev(g_major,  "char_driver"); 
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a char module");
MODULE_ALIAS("a char test module");
