/*
 * a platform module: misc
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


#define G_MEM_SIZE 1024

struct x_dev { 
	struct miscdevice miscdev;
	char buf[G_MEM_SIZE];
};

static struct platform_device *x_pdev;

static int x_open(struct inode *inode, struct file *filp)
{
	struct x_dev *dev = container_of(filp->private_data, struct x_dev, miscdev);

	printk(KERN_INFO "%s, dev:%smajor:%d minor:%d\n", __func__, 
				dev->miscdev.name, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
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
	//获取打开文件对应的设备
	struct x_dev *dev = filp->private_data;

	//操作位置到文件尾，或超出文件尾了
	if (p > G_MEM_SIZE)
	  return 0;

	//在当前位置所要读的数目超过文件尾了
	if (cnt > G_MEM_SIZE - p)
	  cnt = G_MEM_SIZE - p;

	if (copy_to_user(buf, dev->buf + p, cnt)) {
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
	struct x_dev *dev = filp->private_data;

	if (p > G_MEM_SIZE)
	  return 0;
	if (cnt > G_MEM_SIZE - p)
	  cnt = G_MEM_SIZE - p;

	if (copy_from_user(dev->buf + p, buf, cnt)) {
		ret = -EFAULT;
	} else {
		*f_pos += cnt;
		ret = cnt;
	}

	return ret;
}


static const struct file_operations xmisc_fops = { 
	.owner      = THIS_MODULE,
	.write      = x_write,
	.read		= x_read,
	.open       = x_open,
	.release    = x_release,
};


//当注册的device和driver名字相同时会调用probe函数
static int x_drv_probe(struct platform_device *dev)
{
	struct x_dev *dev_xxx;
	int ret;
	printk(KERN_INFO "%s\n", __func__);

	dev_xxx = kzalloc(sizeof(struct x_dev), GFP_KERNEL);
	if (!dev_xxx)
	  return -ENOMEM;

	dev_xxx->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev_xxx->miscdev.name = "xmisc";
	dev_xxx->miscdev.fops = &xmisc_fops;

	platform_set_drvdata(dev, dev_xxx);

	ret = misc_register(&dev_xxx->miscdev);
	if (ret < 0)
	  return -ENODEV;

	return 0;
}

static int x_drv_remove(struct platform_device *dev)
{
	struct x_dev *priv = platform_get_drvdata(dev);

	misc_deregister(&priv->miscdev);
	kfree(priv);
	printk(KERN_INFO "%s end\n", __func__);

	return 0;
}

//static int x_resume(struct platform_device *dev)
//static int x_suspend(struct platform_device *dev, pm_message_t state)

static struct platform_driver x_driver = {
	.probe		= x_drv_probe,
	.remove		= x_drv_remove,
	//	.suspend  = x_drv_suspend,
	//	.resume   = x_drv_resume,
	.driver		= {
		.name	= "x_dev", //driver的名字为x_dev
		.owner = THIS_MODULE,
	},
};

static int __init x_init(void)
{
	int ret;

	x_pdev = platform_device_alloc("x_dev", -1);//device的名字为x_dev
	if (!x_pdev) {
		return -ENOMEM;
	}

	ret = platform_device_add(x_pdev);  //注册设备
	if (ret < 0) {
		goto err;
	}

	ret = platform_driver_register(&x_driver); //注册驱动
	if (ret) {
		goto err;	
	}

	printk(KERN_INFO "%s end\n", __func__);

	return 0;
err:
	platform_device_put(x_pdev);
	return ret;
}


static void __exit x_exit(void)
{
	platform_device_unregister(x_pdev);
	platform_driver_unregister(&x_driver);
	printk(KERN_INFO "%s end\n", __func__);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a misc module");
MODULE_ALIAS("a misc module");
