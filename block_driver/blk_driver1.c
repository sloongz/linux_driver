/*
 * a simple kernel module: blk_driver 
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/spinlock.h>


//主设备号
#define X_MAJOR 222
#define DEVICE_NAME "mblk_dev"
#define MBLK_BYTES (16*1024*1024) //16M	

static struct gendisk *mblk_gendisk;
static struct request_queue *mblk_queue;
static unsigned char mblk_data[MBLK_BYTES];

static void do_mblk_request(struct request_queue *q)
{
	struct request *req;
	unsigned long sector, nr_sectors;
	unsigned long offset, nbytes;
	int err = 0;
	void *buffer;

	printk(KERN_INFO "enter simple request routine\n");

	//从请求队列拿出一条请求
	req = blk_fetch_request(q);
	while (req) {
		sector = blk_rq_pos(req);
		nr_sectors = blk_rq_cur_sectors(req);
		offset = sector << 9;
		nbytes = nr_sectors << 9;
		//nbytes = blk_rq_cur_bytes(req);

		if (offset + nbytes > MBLK_BYTES) {
			printk(KERN_ERR "%s:bad access: block:%llu, count=%u\n",
						DEVICE_NAME,
						(unsigned long long)blk_rq_pos(req),
						blk_rq_cur_sectors(req));
			err = -EIO;
			goto done;
		}


		buffer = bio_data(req->bio);
		if (rq_data_dir(req) == READ) {
			printk("read offset:%lu size:%lu bytes\n", offset, nbytes);
			memcpy(buffer, mblk_data + offset, nbytes);
		} else { //WRITE 
			printk("write offset:%lu size:%lu bytes\n", offset, nbytes);
			memcpy(mblk_data + offset, buffer, nbytes);
		}

	done:
		if (!__blk_end_request_cur(req, err))
			req = blk_fetch_request(q);
	}
}

static int mblk_open(struct block_device *bdev, fmode_t mode)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

static void mblk_release(struct gendisk *disk, fmode_t mode)
{
	printk(KERN_INFO "%s\n", __func__);
}

static const  struct block_device_operations mblk_fops = {
	.owner = THIS_MODULE,	
	.open = mblk_open,
	.release = mblk_release,
};

static int __init xblk_init(void)
{
	int ret;
	spinlock_t lock;
	spin_lock_init(&lock);

	printk(KERN_INFO "blk init\n");

	memset(mblk_data, 0, MBLK_BYTES);

	if (register_blkdev(X_MAJOR, DEVICE_NAME)) {
		ret = -ENOMEM;
		goto err;
	}

	mblk_gendisk = alloc_disk(1);
	if (!mblk_gendisk) {
		ret = -ENOMEM;
		goto out_disk;
	}

	//I/O调度器把排序后的访问需求通过request_queue结构传递给块设备驱动程序处理
	mblk_queue = blk_init_queue(do_mblk_request, NULL);
	if (!mblk_queue) {
		ret = ENOMEM;
		goto out_queue;
	}

	mblk_gendisk->major = X_MAJOR;
	mblk_gendisk->first_minor = 0;
	mblk_gendisk->fops = &mblk_fops;
	sprintf(mblk_gendisk->disk_name, "mblk_dev");
	mblk_gendisk->queue = mblk_queue;
	set_capacity(mblk_gendisk, MBLK_BYTES>>9); //假设一个扇区512Bytes

	add_disk(mblk_gendisk);

	return 0;
out_queue:
	del_gendisk(mblk_gendisk);
out_disk:
	unregister_blkdev(X_MAJOR, DEVICE_NAME);
err:
	return ret;
}


static void __exit xblk_exit(void)
{
	printk(KERN_INFO "blk exit\n");
	unregister_blkdev(X_MAJOR, DEVICE_NAME);
	del_gendisk(mblk_gendisk);
	put_disk(mblk_gendisk);
	blk_cleanup_queue(mblk_queue);
}

module_init(xblk_init);
module_exit(xblk_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
