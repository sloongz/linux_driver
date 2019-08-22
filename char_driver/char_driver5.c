/*
 * a char kernel module: char
 *
 * Copyright (C) 2019 sloongz
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
#include <linux/spinlock.h>
#include <linux/atomic.h>

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


//#define DEBUG_SPIN_LOCK
#define DEBUG_MUTEX
//#define DEBUG_SEMA
//#define DEBUG_COMPL

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
	struct semaphore sem_mutex; //信号量用于互斥
	struct semaphore sem_read; //信号量用于同步
	struct completion compl; //完成量
	int f_open_cnt;
	atomic_t f_open_cnt1; //原子计数
	spinlock_t lock;
};


struct g_dev *g_devp;

static int x_open(struct inode *inode, struct file *filp)
{
	struct g_dev *dev = container_of(inode->i_cdev, struct g_dev, cdev);

    //私有数据指针， 大多数指向驱动中自定义的设备结构体
	filp->private_data = dev;
#ifdef DEBUG_SPIN_LOCK //打开这个宏， 文件只能被打开一次
	spin_lock(&dev->lock);
	if (dev->f_open_cnt) { //已经打开
		printk("file has been open\n");
		spin_unlock(&dev->lock);
		return - EBUSY;	
	}
	dev->f_open_cnt++;//增加使用计数
	spin_unlock(&dev->lock);
#endif

	atomic_inc(&dev->f_open_cnt1);

	printk(KERN_INFO "%s, major:%d minor:%d\n", __func__, 
				MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int x_release(struct inode *inode, struct file *filp)
{
	struct g_dev *dev = container_of(inode->i_cdev, struct g_dev, cdev);

#ifdef DEBUG_SPIN_LOCK
	spin_lock(&dev->lock);
	dev->f_open_cnt--;//减少使用计数
	spin_unlock(&dev->lock);
#endif

	atomic_dec(&dev->f_open_cnt1);

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
	struct g_dev *dev = filp->private_data;

	printk(KERN_INFO "%s offset:%ld, line:%d\n", __func__, p, __LINE__);

#ifdef DEBUG_SEMA
	//获取不到信号量就睡， 可被中断打断
	//如果没有写操作， 读就会睡眠等待， 直到有写操作把信号量sem_read +1
	if(down_interruptible(&dev->sem_read)) { 
		return -ERESTARTSYS;
	}
	printk(KERN_INFO "read func semaphore P\n");
#endif

#ifdef DEBUG_COMPL
	wait_for_completion(&dev->compl);//信号不能打断
#endif
	
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
	//mutex_lock(&dev->mutex);
	down(&dev->sem_mutex); //使用down替代mutex_lock
	if (copy_to_user(buf, dev->buf + p, cnt)) {
		ret = -EFAULT;	
	} else {
		*f_pos += cnt;
		ret = cnt;
	}
	//mutex_unlock(&dev->mutex);
	up(&dev->sem_mutex);//使用up替代mutex_unlock
	
	return ret;
}

static ssize_t x_write(struct file *filp, const char __user *buf,
			size_t size, loff_t *f_pos)
{
	int ret = 0;
	unsigned int cnt = size;
	unsigned long p = *f_pos;
	struct g_dev *dev = filp->private_data;

	if (p > G_MEM_SIZE)
        return 0;
	if (cnt > G_MEM_SIZE - p)
		cnt = G_MEM_SIZE - p;

	//保护临界区
	//mutex_lock(&dev->mutex);
	down(&dev->sem_mutex);
	if (copy_from_user(dev->buf + p, buf, cnt)) {
		ret = -EFAULT;
	} else {
		*f_pos += cnt;
		ret = cnt;
	}

	if (dev->size < *f_pos)
		dev->size = *f_pos;

#ifdef DEBUG_MUTEX
	//测试mutex, 如果用户写入sleep字符串则休眠10秒在解锁
	if (!strncmp("sleep", buf, 5)) {
		msleep(1000*10);	
		printk(KERN_INFO "sleep over\n");
	}
#endif

	//mutex_unlock(&dev->mutex);
	up(&dev->sem_mutex); //信号量+1

#ifdef DEBUG_SEMA
	printk(KERN_INFO "write func semaphore V\n");
	up(&dev->sem_read);
#endif

#ifdef DEBUG_COMPL
	complete(&dev->compl);
#endif

	return ret;
}

static long x_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct g_dev *dev = filp->private_data;	

	if (_IOC_TYPE(cmd) != X_MAGIC) {
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > X_MAX_NR) {
		return -ENOTTY;
	}

	switch (cmd) {
	case X_IO_SET:
		printk(KERN_INFO "%s X_IO_SET\n", __func__);
		//mutex_lock(&dev->mutex);
		down(&dev->sem_mutex);
		memset(dev->buf, 's', G_MEM_SIZE);
		//mutex_unlock(&dev->mutex);
		up(&dev->sem_mutex);
		break;
	case X_IO_CLEAR:
		printk(KERN_INFO "%s X_IO_CLEAR\n", __func__);
		//mutex_lock(&dev->mutex);
		down(&dev->sem_mutex);
		memset(dev->buf, 0, G_MEM_SIZE);
		//mutex_unlock(&dev->mutex);
		up(&dev->sem_mutex);
		break;
	case X_IOR:
		printk(KERN_INFO "%s X_IOR, dev minor:%d\n", __func__, MINOR(dev->cdev.dev));
		//mutex_lock(&dev->mutex);
		down(&dev->sem_mutex);
		if (copy_to_user((char *)arg, dev->buf, G_MEM_SIZE)) {
			return -EFAULT;
		}	
		//mutex_unlock(&dev->mutex);
		up(&dev->sem_mutex);
		break;
	case X_IOW:
		printk(KERN_INFO "%s X_IOW\n", __func__);
		//mutex_lock(&dev->mutex);
		down(&dev->sem_mutex);
		if (copy_from_user(dev->buf, (char *)arg, G_MEM_SIZE)) {
			return -EFAULT;
		}
		//mutex_unlock(&dev->mutex);
		up(&dev->sem_mutex);
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
	printk(KERN_INFO "%s offset:%lld\n", __func__, newpos);

	return newpos;
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
		sema_init(&(g_devp + i)->sem_mutex, 1); //初始化信号量用于互斥
		sema_init(&(g_devp + i)->sem_read, 0); //初始化信号量用于同步
		init_completion(&(g_devp + i)->compl);
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
