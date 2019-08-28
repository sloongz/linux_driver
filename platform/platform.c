/*
 * a platform module: platform
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>

struct x_dev { 
	int cnt;
	struct work_struct work;
};

static struct platform_device *x_pdev;

static void work_func(struct work_struct *work)
{
	struct x_dev *devp = container_of(work, struct x_dev, work);

	printk(KERN_INFO "%s, cnt:%d\n", __func__, devp->cnt++);	
	msleep(1*1000);	
	schedule_work(&devp->work);//调度任务
}

//当注册的device和driver名字相同时会调用probe函数
static int x_drv_probe(struct platform_device *dev)
{
	struct x_dev *dev_xxx;
	printk(KERN_INFO "%s\n", __func__);

	dev_xxx = kmalloc(sizeof(struct x_dev), GFP_KERNEL);
	if (!dev_xxx)
		return -ENOMEM;

	memset(dev_xxx, 0, sizeof(struct x_dev));

	platform_set_drvdata(dev, dev_xxx);

	INIT_WORK(&dev_xxx->work, work_func);
	schedule_work(&dev_xxx->work);

	return 0;
}

static int x_drv_remove(struct platform_device *dev)
{
	struct x_dev *dev_xxx = platform_get_drvdata(dev);

	cancel_work_sync(&dev_xxx->work);
	kfree(dev_xxx);
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
MODULE_DESCRIPTION("a platform module");
MODULE_ALIAS("a platform module");
