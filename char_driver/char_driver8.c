/*
 * a char kernel module: char
 *
 * Copyright (C) 2018 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/poll.h>

//主设备号
#define X_MAJOR 222

#define G_MEM_SIZE 1024

#define DEVICE_NUM	5

#define X_MAGIC   'X'
#define X_MAX_NR  4
#define X_IO_SET	_IO(X_MAGIC, 0)
#define X_IO_CLEAR	_IO(X_MAGIC, 1)
#define X_IOR		_IOR(X_MAGIC, 2, char[G_MEM_SIZE])
#define X_IOW		_IOW(X_MAGIC, 3, char[G_MEM_SIZE])

static int g_major = X_MAJOR;
module_param(g_major, int, S_IRUGO);
//module_param(name, type, perm);
//其中,name:表示参数的名字;
//     type:表示参数的类型;
//     perm:表示参数的访问权限;

struct g_dev {
	struct cdev cdev;
	int size;
	char buf[G_MEM_SIZE];
	struct mutex mutex; //互斥信号量
	wait_queue_head_t r_wait; //阻塞读
	wait_queue_head_t w_wait; //阻塞写
	struct fasync_struct *async_queue; //异步结构
};

struct g_dev *g_devp;

static int x_fasync(int fd, struct file *filp, int mode)
{
	struct g_dev *dev = filp->private_data;

	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int x_open(struct inode *inode, struct file *filp)
{
	struct g_dev *dev = container_of(inode->i_cdev, struct g_dev, cdev);

    //私有数据指针， 大多数指向驱动中自定义的设备结构体
	filp->private_data = dev;

	if (filp->f_flags & O_NONBLOCK) {
		printk(KERN_INFO "file no block\n");
	} else {
		printk(KERN_INFO "file block\n");
	}

	printk(KERN_INFO "%s, major:%d minor:%d\n", __func__, 
				MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int x_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s\n", __func__);
	//将文件从异步通知列表中删除
	x_fasync(-1, filp, 0);
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
	struct g_dev *dev = filp->private_data;

	printk(KERN_INFO "%s offset:%ld, size:%d\n", __func__, p, dev->size);

	if (!dev->size) {
		//如果设置了O_NONBLOCK，那么直接返回-EAGAIN
		if(filp->f_flags & O_NONBLOCK) {
			printk(KERN_INFO "%s %d, file noblock return\n", __func__, __LINE__);
		    return -EAGAIN;
		}
		//如果设备缓冲区没有数据， 则读操作就等待直到缓冲区写入数据
		if (wait_event_interruptible(dev->r_wait, dev->size != 0)) { //阻塞等待
			return -ERESTARTSYS;
		}
	}

	//操作位置到文件尾，或超出文件尾了
	//if (p > G_MEM_SIZE)
	if (p >= dev->size)
	    return 0;
	  
	//在当前位置所要读的数目超过文件尾了
	//if (cnt > G_MEM_SIZE - p)
	//	cnt = G_MEM_SIZE - p;
	if (p+cnt > dev->size)
		cnt = dev->size - p;

	//保护临界区
	mutex_lock(&dev->mutex);
	if (copy_to_user(buf, dev->buf + p, cnt)) {
		mutex_unlock(&dev->mutex);
		ret = -EFAULT;	
	} else {
		*f_pos += cnt;
		ret = cnt;
		dev->size -= cnt;//如果读数据就减小对应的设备缓存数据
	}
	mutex_unlock(&dev->mutex);

	printk("read: size:%d\n", dev->size);	
	if (dev->size < G_MEM_SIZE) {
		//如果设备缓冲区有空间就通知写操作可以写入
		wake_up_interruptible(&dev->w_wait);
	}

	return ret;
}

static ssize_t x_write(struct file *filp, const char __user *buf,
			size_t size, loff_t *f_pos)
{
	int ret = 0;
	unsigned int cnt = size;
	unsigned long p = *f_pos;
	struct g_dev *dev = filp->private_data;

	if (dev->size >= G_MEM_SIZE) {
		//如果设置了O_NONBLOCK，那么直接返回-EAGAIN
		if(filp->f_flags & O_NONBLOCK) {
			printk(KERN_INFO "%s %d, file noblock return\n", __func__, __LINE__);
		    return -EAGAIN;
		}
		//如果设备缓冲区没有空间， 则写操作就等待直到缓冲区有空间为止
		if (wait_event_interruptible(dev->w_wait, dev->size < G_MEM_SIZE)) { //阻塞等待
			return -ERESTARTSYS;
		}
	}

	if (p > G_MEM_SIZE)
        return 0;
	if (cnt > G_MEM_SIZE - p)
		cnt = G_MEM_SIZE - p;

	//保护临界区
	mutex_lock(&dev->mutex);
	if (copy_from_user(dev->buf + p, buf, cnt)) {
		mutex_unlock(&dev->mutex);
		ret = -EFAULT;
	} else {
		*f_pos += cnt;
		ret = cnt;
	}

	if (dev->size < *f_pos)
		dev->size = *f_pos;//更新设备缓冲区中数据的大小

	mutex_unlock(&dev->mutex);

	printk("write: size:%d\n", dev->size);
	if (dev->size > 0) {
		//如果设备缓冲区有数据， 就通知读操作可以读数据
		wake_up_interruptible(&dev->r_wait);
		if (dev->async_queue) {
			//产生异步读信号
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
		}
	}
	return ret;
}

static long x_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct g_dev *dev = filp->private_data;	
	int ret = 0;

	if (_IOC_TYPE(cmd) != X_MAGIC) {
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > X_MAX_NR) {
		return -ENOTTY;
	}

	switch (cmd) {
	case X_IO_SET:
		printk(KERN_INFO "%s X_IO_SET\n", __func__);
		mutex_lock(&dev->mutex);
		memset(dev->buf, 's', G_MEM_SIZE);
		dev->size = G_MEM_SIZE;
		mutex_unlock(&dev->mutex);
		break;
	case X_IO_CLEAR:
		printk(KERN_INFO "%s X_IO_CLEAR\n", __func__);
		mutex_lock(&dev->mutex);
		memset(dev->buf, 0, G_MEM_SIZE);
		dev->size = 0;
		mutex_unlock(&dev->mutex);
		break;
	case X_IOR:
		printk(KERN_INFO "%s X_IOR, dev minor:%d\n", __func__, MINOR(dev->cdev.dev));
		mutex_lock(&dev->mutex);
		if (copy_to_user((char *)arg, dev->buf, G_MEM_SIZE)) {
			return -EFAULT;
		}	
		mutex_unlock(&dev->mutex);
		break;
	case X_IOW:
		printk(KERN_INFO "%s X_IOW\n", __func__);
		mutex_lock(&dev->mutex);
		ret = copy_from_user(dev->buf, (char *)arg, G_MEM_SIZE);
		dev->size = G_MEM_SIZE - ret;
		mutex_unlock(&dev->mutex);
		printk("app data:%s\n", dev->buf);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static loff_t x_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t newpos;
	struct g_dev *dev = filp->private_data;

	switch (whence) {
	case 0: //SEEK_SET
		newpos = offset;
		break;
	case 1: //SEEK_CUR
		newpos = filp->f_pos + offset;
		break;
	case 2: //SEEK_END
		newpos = dev->size + offset;
		break;
	default:
		newpos = -EINVAL;
	}

	if (newpos < 0 || newpos > G_MEM_SIZE) 
		return -EINVAL;

	filp->f_pos = newpos;
	mutex_lock(&dev->mutex);
	dev->size = newpos;
	mutex_unlock(&dev->mutex);
	printk("%s offset:%lld\n", __func__, newpos);

	return newpos;
}

static unsigned int x_poll(struct file *filp, poll_table * wait)
{
	struct g_dev *dev = filp->private_data;
	unsigned int mask = 0;

	printk(KERN_INFO "%s start\n", __func__);

	mutex_lock(&dev->mutex);
	//将等待队列r_wait加入poll_table中, poll_wait()函数不会阻塞
	poll_wait(filp, &dev->r_wait, wait);
	//将等待队列r_wait加入poll_table中
	poll_wait(filp, &dev->w_wait, wait);

	//根据数据状态设置状态掩码
	if (dev->size > 0) {//如果缓冲区有数据就可读
		mask |= POLLIN | POLLRDNORM;
		printk("poll read flag mask=0x%x\n", mask);
	}

	if (dev->size < G_MEM_SIZE) {//如果缓冲区没满就可写
		mask |= POLLOUT | POLLWRNORM;
		printk("poll write flag mask=0x%x\n", mask);
	}
	mutex_unlock(&dev->mutex);
	printk(KERN_INFO "%s end\n", __func__);

	return mask;
}


//填充file_operations结构体
static struct file_operations x_fops = {  
	.owner =    THIS_MODULE,  
	.open =  x_open,  
	.release =  x_release,  
	.read =     x_read,  
	.write =    x_write,
	.unlocked_ioctl = x_ioctl,
	.llseek = x_llseek,
	.poll = x_poll, 
	.fasync = x_fasync,
}; 

static void setup_cdev(struct g_dev *dev, int index)
{
	int ret;
	//#define MKDEV(major,minor) (((major) << MINORBITS) | (minor))
	//成功执行返回dev_t类型的设备编号，dev_t类型是unsigned int 类型，32位，
	//用于在驱动程序中定义设备编号，高12位为主设备号，低20位为次设备号
	//可以通过MAJOR和MINOR来获得主设备号和次设备号。
	int devno = MKDEV(g_major, index);

	//将struct cdev类型的结构体变量和file_operations结构体进行绑定的
	cdev_init(&dev->cdev, &x_fops);
	dev->cdev.owner = THIS_MODULE;
	//向内核里面添加一个驱动，注册驱动
	//第三个参数：和该设备关联的设备编号的数量
	ret = cdev_add(&dev->cdev, devno, 1);
	if (ret) {
		printk("create index %d device fail\n", index);
	}
}


//模块初始化
static int __init x_init(void)
{
	int ret;
	int i;
	dev_t devno = MKDEV(g_major, 0);

	printk(KERN_INFO "char enter\n");

	//register_chrdev  比较老的内核注册的形式，早期的驱动
	//register_chrdev_region/alloc_chrdev_region + cdev，新的驱动形式
	//函数参数:
	//devno 设备编号主设备号次设备号组成
	//DEVICE_NUM 次设备号个数
	//设备名称 /proc/devices 下显示的名字 char_driver
	if (g_major) {
		ret = register_chrdev_region(devno, DEVICE_NUM, "char_driver");	
	} else {
		//第二个参数 0 表示次设备号的基准，从第几个次设备号开始分配
		ret = alloc_chrdev_region(&devno, 0, DEVICE_NUM, "char_driver");
		g_major = MAJOR(devno);
	}
	if (ret < 0) {
		printk(KERN_ERR "unable to register char dev %d\n", g_major);
		return ret;
	}

	g_devp = kzalloc(sizeof(struct g_dev) * DEVICE_NUM, GFP_KERNEL);
	if (!g_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}

	for (i=0; i<DEVICE_NUM; i++) {
		setup_cdev(g_devp + i, i);
		(g_devp + i)->size = 0;
		mutex_init(&(g_devp + i)->mutex);
		init_waitqueue_head(&(g_devp + i)->r_wait);
		init_waitqueue_head(&(g_devp + i)->w_wait);
	}

	return 0;

fail_malloc:
	unregister_chrdev_region(devno, DEVICE_NUM);
	return ret;
}

//模块注销
static void __exit x_exit(void)
{
	int i;
	printk(KERN_INFO "char exit\n");
	//注销字符设备
	//unregister_chrdev(g_major,  "char_driver"); 
	for (i=0; i<DEVICE_NUM; i++) {
		cdev_del(&(g_devp + i)->cdev);
	}
	kfree(g_devp);
	unregister_chrdev_region(MKDEV(g_major, 0), DEVICE_NUM);
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a char module");
MODULE_ALIAS("a char test module");
