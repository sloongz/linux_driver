
##### 块设备概念

块设备就是指磁盘、CD-ROM等硬件存储介质， 块设备驱动连接了块设备和用户空间，实现用户空间对磁盘的大块数据访问。整个子系统如下图所示：

![linuxIO_image](https://github.com/sloongz/linux_driver/blob/master/Image/block_linuxIO.jpg?raw=true)

包括悉尼文件系统、块IO调度层，块设被驱动以及具体的块设备。块设备不同于字符设备，它是以块为单位接收输入和返回输出，而字符设备是以字节为单位。块设备支持随机访问。块是最小的读写单位，不同的文件系统有不同大小的块尺寸，但是它必须是2的指数，同时不能超过页大小。通常使用的大小有512字节，1K字节，4K字节等。

虚拟文件系统（VFS）：隐藏了各种硬件的具体细节，为用户操作不同的硬件提供了一个统一的接口。其基于不同的文件系统格式，比如EXT，FAT等。用户程序对设备的操作都通过VFS来完成，在VFS上面就是诸如open、close、write和read的函数API

映射层（mapping layer）：这一层主要用于确定文件系统的block size，然后计算请求的数据包含多少block。同时调用具体文件系统函数来访问文件的inode，确定所有请求的数据在磁盘上面的逻辑地址。

IO调度器：这部分是Linux块系统非常关键的部分，其涉及到如何接收用户请求并高效去访问硬件磁盘中的数据。

Block driver：完成和块设备的具体交互。

##### 相关API和数据结构：

block_device_operations类似于字符设备驱动中的file_operations结构，它是对块设备各种操作的集合，定义代码如下：

```
struct block_device_operations {
int (*open) (struct block_device *, fmode_t);
int (*release) (struct gendisk *, fmode_t);
int (*locked_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
int (*direct_access) (struct block_device *, sector_t,void **, unsigned long *);
int (*media_changed) (struct gendisk *);
int (*revalidate_disk) (struct gendisk *);
int (*getgeo)(struct block_device *, struct hd_geometry *);
struct module *owner;
};
```
介质改变

```
int (*check_media_change) (kdev_t);

int (*revalidate) (kdev_t);
```
像磁盘、CD-ROM等块设备是可插拔的，因此需要有个函数来检测设备是否存在。当介质发生改变，使用revalidate_disk来响应，给驱动一个机会进行必要的工作来使介质准备好。

 获得驱动信息
 
```
int (*getgeo)(struct block_device *,struct hd_geometry *)
```
该函数根据驱动器的几何信息填充一个hd_geometry结构体，hd_geometry包含磁头、扇区、柱面等信息。

##### gendisk结构体

在linux内核中，用gendisk结构体来表示一个独立的磁盘设备。就像字符设备驱动中使用cdev结构体一样，它也包含主次设备号，需要分配内存，释放结构体和初始化操作。

1. 分配gendisk

gendisk 结构体是一个动态的结构体， 他需要特别的内核操作来初始化，驱动不能自己分配这个结构体，而应该使用下面函数分配

```
struct gendisk *alloc_disk(int minors);
```
minors 参数是这个磁盘使用的次设备号的数量，一般也就是磁盘分区的数量，此后minors不能被修改。

2. 增加gendisk

gendisk结构体被分配之后，系统还不能使用这个磁盘，需要调用如下函数来注册这个磁盘设备。

```
void add_disk(struct gendisk *disk);
```

3. 释放gendisk

当不再需要磁盘时， 应当使用如下函数释放gendisk

```
void del_gendisk(struct gendisk *gd);
```

4. 设置gendisk

```
static inline void set_capacity(struct gendisk *disk, sector_t size)
```
告诉内核这个设备有多少块。 一般是BLKDEV_SIZE >> 9 (512Bytes一个块)

##### bio、request和request_queue

通常一个bio对应上层传递给块层的IO请求。每个bio结构体实例及其包含的bvec_iter、bio_vec结构体实例描述了该IO请求的开始扇区、数据方向（R/W）、数据放入的页。


```
/*
 * main unit of I/O for the block layer and lower layers (ie drivers and
 * stacking drivers)
 */
struct bio {
    struct bio      *bi_next;   /* request queue link */
    struct block_device *bi_bdev;
    unsigned long       bi_flags;   /* status, command, etc */
    unsigned long       bi_rw;      /* bottom bits READ/WRITE, top bits priority*/
    struct bvec_iter    bi_iter;
    /* Number of segments in this BIO after
     * physical address coalescing is performed.
     */
    unsigned int        bi_phys_segments;
    /*
     * To keep track of the max segment size, we account for the
     * sizes of the first and last mergeable segments in this bio.
     */
    unsigned int        bi_seg_front_size;
    unsigned int        bi_seg_back_size;
    atomic_t        bi_remaining;
    bio_end_io_t        *bi_end_io;
    void            *bi_private;
#ifdef CONFIG_BLK_CGROUP
    /*
     * Optional ioc and css associated with this bio.  Put on bio
     * release.  Read comment on top of bio_associate_current().
     */
    struct io_context   *bi_ioc;
    struct cgroup_subsys_state *bi_css;
#endif
    union {
#if defined(CONFIG_BLK_DEV_INTEGRITY)
        struct bio_integrity_payload *bi_integrity; /* data integrity */
#endif
    };
    unsigned short      bi_vcnt;    /* how many bio_vec's */
    /*
     * Everything starting with bi_max_vecs will be preserved by bio_reset()
     */
    unsigned short      bi_max_vecs;    /* max bvl_vecs we can hold */
    atomic_t        bi_cnt;     /* pin count */
    struct bio_vec      *bi_io_vec; /* the actual vec list */
    struct bio_set      *bi_pool;
    /*
     * We can inline a number of vecs at the end of the bio, to avoid
     * double allocations for a small number of bio_vecs. This member
     * MUST obviously be kept at the very end of the bio.
     */
    struct bio_vec      bi_inline_vecs[0];
};

```

```
struct bvec_iter {
    sector_t        bi_sector;  /* device address in 512 byte sectors */    要操作的扇区号
    unsigned int    bi_size;    /* residual I/O count */                    剩余的bio_vec数目
    unsigned int    bi_idx;     /* current index into bvl_vec */            当前的bio_vec的索引号
    unsigned int    bi_bvec_done;   /* number of bytes completed in current bvec */ 当前bio_vec中已完成的字节数
};
```

```
struct bio_vec {
    struct page *bv_page;       //数据所在页面的首地址
    unsigned int    bv_len;     //数据长度  
    unsigned int    bv_offset;  //页面偏移量
};
```



bio是一个描述硬盘里面的位置与page cache的页对应关系的数据结构， 每个bio 对应硬盘里面的一块连续的位置，每一块硬盘里面连续的位置， 可能对应着page cache的多页，或者一页，所以它里面会有一个bio_vec *bi_io_vec的表。


用户每进行一次对硬盘的操作,都会被操作系统处理成一个请求, 然后放入相应的请求队列中(该请求队列由驱动定义),一个请求可以包含若干个bio,一个bio又可以包含若干个bio_vec。

通过bio, 再结合其中的bio_iter就可以找到当前的bio_vec.

![BIO](https://github.com/sloongz/linux_driver/blob/master/Image/block_BIO.png?raw=true)

IO调度算法可将连续的bio合并成一个请求。请求是bio经由IO调度进行调整后的结果。当bio被提交给IO调度器时，IO调度器可能会将这个bio插入现存的请求中，也可能生成新的请求。

每个块设备或者块设备的分区都对应有自身的request_queue，从IO调度器合并和排序出来的请求会被分发（dispatch）到设备级的request_queue。

相关API

1. 初始化请求队列

```
struct request_queue *blk_init_queue(request_fn_proc *rfn, spinlock_t *lock)
```
rfn 请求处理函数的指针

lock 控制队列访问权限的自旋锁。

2. 清除请求队列

```
void blk_cleanup_queue(struct request_queue *q)
```

3. 分配请求队列

```
struct request_queue *blk_alloc_queue(gfp_t gfp_mask)
```

对于ramdisk这种完全随机访问的非机械设备， 并不需要进行复杂的IO调度，这时候， 可以直接“踢开”IO调度器，使用如下函数来绑定请求队列和“制造请求”函数（make_request_fn）。

```
void blk_queue_make_request(struct request_queue *q, make_request_fn *mfn)
```

配合使用：

```
xxx_queue = blk_alloc_queue(GFP_KERNEL);
blk_queue_make_request(xxx_queue, xxx_make_request);
```

4. 提取并启动请求请求

```
struct request *blk_fetch_request(struct request_queue *q)
{
    struct request *rq; 

    rq = blk_peek_request(q);
    if (rq) 
        blk_start_request(rq);
    return rq;
}
```

5. 遍历bio和片段

遍历一个请求的所有bio

__rq_for_each_bio
```
#define __rq_for_each_bio(_bio, rq) \
    if ((rq->bio))          \
        for (_bio = (rq)->bio; _bio; _bio = _bio->bi_next)

```
遍历一个bio的所有bio_vec

bio_for_each_segment
```
#define __bio_for_each_segment(bvl, bio, iter, start)           \
    for (iter = (start);                        \
         (iter).bi_size &&                      \
        ((bvl = bio_iter_iovec((bio), (iter))), 1);     \
         bio_advance_iter((bio), &(iter), (bvl).bv_len))

#define bio_for_each_segment(bvl, bio, iter)                \
    __bio_for_each_segment(bvl, bio, iter, (bio)->bi_iter)
```

迭代遍历一个请求所有bio中的所有segment

```
#define rq_for_each_segment(bvl, _rq, _iter)            \
    __rq_for_each_bio(_iter.bio, _rq)           \
        bio_for_each_segment(bvl, _iter.bio, _iter.iter)
```

6. 完成报告

```
/*
 * Request completion related functions.
 *
 * blk_update_request() completes given number of bytes and updates
 * the request without completing it.
 *
 * blk_end_request() and friends.  __blk_end_request() must be called
 * with the request queue spinlock acquired.
 *
 * Several drivers define their own end_request and call
 * blk_end_request() for parts of the original function.
 * This prevents code duplication in drivers.
 */ 
extern bool blk_update_request(struct request *rq, int error,
                   unsigned int nr_bytes);
extern void blk_finish_request(struct request *rq, int error);
extern bool blk_end_request(struct request *rq, int error,
                unsigned int nr_bytes);
extern void blk_end_request_all(struct request *rq, int error); 
extern bool blk_end_request_cur(struct request *rq, int error);
extern bool blk_end_request_err(struct request *rq, int error);
extern bool __blk_end_request(struct request *rq, int error,
                  unsigned int nr_bytes);
extern void __blk_end_request_all(struct request *rq, int error);
extern bool __blk_end_request_cur(struct request *rq, int error);
extern bool __blk_end_request_err(struct request *rq, int error);
```

error为0表示成功， 小于0表示失败。其中xxx_end_request_cur()只能完成request中当前的那个chunk。也就是完成了当前的bio_cur_bytes(req->bio)的传输。

若我们用blk_queue_make_request()绕开IO调度，但是在bio处理完成后应该使用bio_endio()函数通知处理结束：

```
void bio_endio(struct bio *bio, int error)
```
如果是IO操作故障，可以调用快捷函数bio_io_error()

```
#define bio_io_error(bio) bio_endio((bio), -EIO)
```



先写一个简单的block device driver， 使用系统默认的IO调度器。然后在慢慢完善。

代码：

https://github.com/sloongz/linux_driver/blob/master/block_driver/blk_driver1.c

make 生成ko

插入驱动

```
$ sudo insmod blk_driver1.ko
```
查看设备文件
```
$ ls /dev/mblk_dev -l
brw-rw---- 1 root disk 222, 0 Aug 12 20:09 /dev/mblk_dev

```
如果没有自动建立mblk_dev设备文件则需要手动建立文件

```
mknod /dev/mblkd_ev b 222 0
```
在设备中创建文件系统

```
$ sudo mkfs.ext4 /dev/mblk_dev 
mke2fs 1.42.13 (17-May-2015)
Creating filesystem with 16384 1k blocks and 4096 inodes
Filesystem UUID: 9ea4547e-c02c-425d-ab2f-4d14771d8357
Superblock backups stored on blocks: 
	8193

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

```

挂载

```
$ mkdir AAA
$ sudo mount /dev/mblk_dev AAA
```
查看

```
$mount
...
/dev/mblk_dev on /home/sean/github/linux_driver/block_driver/AAA type ext4 (rw,relatime,data=ordered)

$ df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/mblk_dev    15M  140K   14M   2% /home/sean/github/linux_driver/block_driver/AAA

```

在目录下创建文件删除文件

```
AAA$ sudo touch 1
AAA$ sudo chmod 777 1
AAA$ sudo echo xxxx > 1
AAA$ ls -l
total 13
-rwxrwxrwx 1 root root     5 Aug 12 20:31 1
drwx------ 2 root root 12288 Aug 12 20:24 lost+found
AAA$ cat 1
xxxx
AAA$ sudo rm 1 
AAA$ ls
lost+found

```

查看使用IO调度算法

```
$ cat /sys/block/mblk_dev/queue/scheduler
noop deadline [cfq] 
```
修改算法

```
$ sudo sh -c "echo noop > /sys/block/mblk_dev/queue/scheduler"
$ cat /sys/block/mblk_dev/queue/scheduler 
[noop] deadline cfq 

```


卸载

```
sudo umount AAA
```
删除驱动

```
$ sudo rmmod blk_driver1
$dmesg
[  971.725268] mblk_release
[  998.678520] blk exit
```
