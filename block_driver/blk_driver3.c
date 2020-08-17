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
#include <linux/bio.h>
#include <linux/hdreg.h>
#include <linux/spinlock.h>

#define X_MAJOR 222 //主设备号
#define DEVICE_NAME "mblk_dev"
#define MBLK_BYTES (16*1024*1024) //16M	
#define MAX_DEV_CNT	3

#define USED_MY_QUEUE 1

struct mdisk_dev {
	struct gendisk *mblk_gendisk;
	struct request_queue *mblk_queue;
	spinlock_t lock;
	unsigned char *mblk_data;
	unsigned long size;
	char name[64];
};

static struct mdisk_dev *g_blkdev;

#ifdef USED_MY_QUEUE 
static unsigned int mblk_make_request(struct request_queue *q, struct bio *bio)
{
	struct bio_vec bvec; 
	struct bvec_iter iter;
	unsigned long long offset;
	void *kaddr;
	static int i = 0;

	if ((bio->bi_iter.bi_sector << 9) + bio->bi_iter.bi_size > MBLK_BYTES) {
		printk(KERN_ERR "%s:bad access: block:%llu, count=%u\n",
				DEVICE_NAME,
				(unsigned long long)bio->bi_iter.bi_sector,
				bio->bi_iter.bi_size);
		bio_io_error(bio);
		return 0;
	}

	offset = bio->bi_iter.bi_sector << 9;

	bio_for_each_segment(bvec, bio, iter) {
		kaddr = kmap(bvec.bv_page);
		if (bio_data_dir(bio) == READ) {
			//printk(KERN_INFO "read bv_len:%u\n", bvec.bv_len);
			memcpy(kaddr+bvec.bv_offset, g_blkdev->mblk_data + offset, bvec.bv_len);	
		} else {
			//printk(KERN_INFO "write bv_len:%u\n", bvec.bv_len);
			memcpy(g_blkdev->mblk_data + offset, kaddr+bvec.bv_offset, bvec.bv_len);	
		} 
		offset += bvec.bv_len;
		kunmap(bvec.bv_page);
	}

	bio_endio(bio);

	return 0;
}
#else
static void do_mblk_request(struct request_queue *q)
{
	struct request *req;
	unsigned long offset, nbytes;
	//unsigned long sector, nr_sectors;
	struct bio_vec bvec;
	struct req_iterator iter;
	int err = 0;
	void *kaddr=NULL;

	while ((req = blk_fetch_request(q)) != NULL) {
		spin_unlock_irq(q->queue_lock);

		offset = blk_rq_pos(req) << 9;
		nbytes = blk_rq_bytes(req);
		
		if (offset + nbytes > MBLK_BYTES) {
			printk(KERN_ERR "%s:bad access: block:%llu, count=%u\n",
						DEVICE_NAME,
						(unsigned long long)blk_rq_pos(req),
						blk_rq_cur_sectors(req));
			err = -EIO;
			goto done;
		}

		rq_for_each_segment(bvec, req, iter) {
			kaddr=kmap(bvec.bv_page);
			if (rq_data_dir(req) == READ) {
				memcpy(kaddr+bvec.bv_offset, g_blkdev->mblk_data + offset, bvec.bv_len);	
			} else { //WRITE			
				memcpy(g_blkdev->mblk_data + offset, kaddr+bvec.bv_offset, bvec.bv_len);	
			} 	
			offset += bvec.bv_len;
			kunmap(bvec.bv_page);
		}

	done:
		blk_end_request_all(req, err);
		spin_lock_irq(q->queue_lock);
	}
}
#endif

static int mblk_open(struct block_device *bdev, fmode_t mode)
{
	struct mdisk_dev *p = bdev->bd_disk->private_data;
	printk(KERN_INFO "%s, open %s device\n", __func__, p->name);
	return 0;
}

static void mblk_release(struct gendisk *disk, fmode_t mode)
{
	struct mdisk_dev *p = disk->private_data;
	printk(KERN_INFO "%s, close %s device\n", __func__, p->name);
}

static int mblk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	printk(KERN_INFO "%s\n", __func__);
	geo->heads = 1;
	geo->sectors = 32;
	geo->cylinders = MBLK_BYTES>>9/geo->heads/geo->sectors;
	return 0;
}

static const  struct block_device_operations mblk_fops = {
	.owner = THIS_MODULE,	
	.open = mblk_open,
	.release = mblk_release,
	.getgeo = mblk_getgeo,
};

static int __init xblk_init(void)
{
	int ret;
	
	printk(KERN_INFO "blk init\n");

	g_blkdev = kmalloc(sizeof(struct mdisk_dev), GFP_KERNEL);
	if (!g_blkdev) {
		goto err;
	} 

	spin_lock_init(&g_blkdev->lock);
	g_blkdev->size = MBLK_BYTES;
	g_blkdev->mblk_data = vmalloc(g_blkdev->size);
	if (!g_blkdev->mblk_data) {
		goto err_mem;
	}

	memcpy(g_blkdev->name, DEVICE_NAME, strlen(DEVICE_NAME)+1);

	if (register_blkdev(X_MAJOR, g_blkdev->name)) {
		ret = -ENOMEM;
		goto err_vmem;
	}

#ifdef USED_MY_QUEUE
	g_blkdev->mblk_queue = blk_alloc_queue(GFP_KERNEL);
	if (!g_blkdev->mblk_queue) {
		ret = -ENOMEM;
		goto out_disk;
	}

	blk_queue_make_request(g_blkdev->mblk_queue, mblk_make_request);
#else
	g_blkdev->mblk_queue = blk_init_queue(do_mblk_request, &g_blkdev->lock);
	if (!g_blkdev->mblk_queue) {
		ret = ENOMEM;
		goto out_disk;
	}
#endif

	g_blkdev->mblk_gendisk = alloc_disk(MAX_DEV_CNT);
	if (!g_blkdev->mblk_gendisk) {
		ret = -ENOMEM;
		goto out_queue;
	}

	g_blkdev->mblk_gendisk->major = X_MAJOR;
	g_blkdev->mblk_gendisk->first_minor = 0;
	g_blkdev->mblk_gendisk->fops = &mblk_fops;
	sprintf(g_blkdev->mblk_gendisk->disk_name, "mblk_dev");
	g_blkdev->mblk_gendisk->queue = g_blkdev->mblk_queue;
	set_capacity(g_blkdev->mblk_gendisk, MBLK_BYTES>>9); //假设一个扇区512Bytes
	g_blkdev->mblk_gendisk->private_data = g_blkdev; 

	add_disk(g_blkdev->mblk_gendisk);

	return 0;
out_queue:
#ifdef USED_MY_QUEUE
	blk_cleanup_queue(g_blkdev->mblk_queue);
#else
	del_gendisk(g_blkdev->mblk_gendisk);
#endif
	put_disk(g_blkdev->mblk_gendisk);
out_disk:
	unregister_blkdev(X_MAJOR, g_blkdev->name);
err_vmem:
	vfree(g_blkdev->mblk_data);
err_mem:
	kfree(g_blkdev);
err:
	return ret;
}

static void __exit xblk_exit(void)
{
	printk(KERN_INFO "blk exit\n");
	unregister_blkdev(X_MAJOR, g_blkdev->name);
	del_gendisk(g_blkdev->mblk_gendisk);
	put_disk(g_blkdev->mblk_gendisk);
	blk_cleanup_queue(g_blkdev->mblk_queue);
	kfree(g_blkdev);
	if (g_blkdev->mblk_data != NULL)
		vfree(g_blkdev->mblk_data);
}

module_init(xblk_init);
module_exit(xblk_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
