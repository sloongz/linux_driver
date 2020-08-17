改进blk_driver1.c
- 增加fops的getgeo 函数
- 修改处理request的函数
- 增加分区数
- 模拟硬盘的内存从数组改为vmalloc申请
- kmap() 和 kunmap()

##### getgeo 描述了磁头、扇区、柱面等信息。

概念：

![image](https://raw.githubusercontent.com/sloongz/linux_driver/master/Image/block_hd.jpg)

盘片（platter）
磁头（head）
磁道（track）
扇区（sector）
柱面（cylinder）

硬盘中一般会有多个盘片组成，每个盘片包含两个面，每个盘面都对应地有一个读/写磁头。受到硬盘整体体积和生产成本的限制，盘片数量都受到限制，一般都在5片以内。盘片的编号自下向上从0开始，如最下边的盘片有0面和1面，再上一个盘片就编号为2面和3面。
如下图：

![image](https://github.com/sloongz/linux_driver/blob/master/Image/block_hd1.png?raw=true)

扇区 和 磁道

下图显示的是一个盘面，盘面中一圈圈灰色同心圆为一条条磁道，从圆心向外画直线，可以将磁道划分为若干个弧段，每个磁道上一个弧段被称之为一个扇区（图践绿色部分）。扇区是磁盘的最小组成单元，通常是512字节。（由于不断提高磁盘的大小，部分厂商设定每个扇区的大小是4096字节）

![image](https://github.com/sloongz/linux_driver/blob/master/Image/block_hd2.png?raw=true)

磁头 和 柱面

硬盘通常由重叠的一组盘片构成，每个盘面都被划分为数目相等的磁道，并从外缘的“0”开始编号，具有相同编号的磁道形成一个圆柱，称之为磁盘的柱面。磁盘的柱面数与一个盘面上的磁道数是相等的。由于每个盘面都有自己的磁头，因此，盘面数等于总的磁头数。 如下图

![image](https://github.com/sloongz/linux_driver/blob/master/Image/block_hd3.png?raw=true)

**存储容量** ＝ 磁头数 × 磁道(柱面)数 × 每道扇区数 × 每扇区字节数。

假设模拟的磁盘有
- 1个磁头
- 32个扇区
- MBLK_BYTES>>9/geo->heads/geo->sectors个柱面
```
static int mblk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
    printk(KERN_INFO "%s\n", __func__);
    geo->heads = 1;
    geo->sectors = 32; 
    geo->cylinders = MBLK_BYTES>>9/geo->heads/geo->sectors;
    return 0;
}
```

##### rq_for_each_segment()
从request 中取出所有的bio_vec。

##### 增加分区数
alloc_disk(1) 只申请了一个子设备
使用fdisk分区， 如果分两个区就会出现错误

```
Device         Boot Start   End Sectors   Size Id Type
/dev/mblk_dev1       2048  3000     953 476.5K 83 Linux
/dev/mblk_dev2       4096 32767   28672    14M 83 Linux

Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Re-reading the partition table failed.: Invalid argument

```
把alloc_disk(MAX_DEV_CNT)申请的子设备数加大， 再分区时就不会出现错误

```
$ sudo fdisk /dev/mblk_dev 

Welcome to fdisk (util-linux 2.27.1).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.

Device does not contain a recognized partition table.
Created a new DOS disklabel with disk identifier 0x59888b87.

Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (1-4, default 1): 
First sector (2048-32767, default 2048): 
Last sector, +sectors or +size{K,M,G,T,P} (2048-32767, default 32767): 4096

Created a new partition 1 of type 'Linux' and of size 1 MiB.

Command (m for help): n
Partition type
   p   primary (1 primary, 0 extended, 3 free)
   e   extended (container for logical partitions)
Select (default p): p
Partition number (2-4, default 2): 
First sector (4097-32767, default 6144): 
Last sector, +sectors or +size{K,M,G,T,P} (6144-32767, default 32767): 

Created a new partition 2 of type 'Linux' and of size 13 MiB.

Command (m for help): p
Disk /dev/mblk_dev: 16 MiB, 16777216 bytes, 32768 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x59888b87

Device         Boot Start   End Sectors Size Id Type
/dev/mblk_dev1       2048  4096    2049   1M 83 Linux
/dev/mblk_dev2       6144 32767   26624  13M 83 Linux

Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Syncing disks.
```

```
$ ls /dev/mblk_dev*
/dev/mblk_dev  /dev/mblk_dev1  /dev/mblk_dev2
```


```
$ sudo mkfs.ext4 /dev/mblk_dev1
$ sudo mkfs.ext4 /dev/mblk_dev2
$ mkdir AAA
$ mkdir BBB
$ sudo mount /dev/mblk_dev1 AAA
$ sudo mount /dev/mblk_dev2 BBB
```


```
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/mblk_dev1 1003K   17K  915K   2% /home/sean/github/linux_driver/block_driver/AAA
/dev/mblk_dev2   12M  116K   11M   2% /home/sean/github/linux_driver/block_driver/BBB
```

##### vmalloc
存开机线性映射了3G-896M的低端内存， 如果是32位机器寻址4G空间， 剩下的128M为高端内存， vmalloc可以在这128M内存上申请虚拟地址连续但是物理地址不连续的内存， 申请过程可能引起睡眠。


##### kmap() 和 kunmap()
高端内存需要在进行访问之前被映射到非线性映射区域， 还要在访问之后解除这个映射。

我们可以用kmap() 和 kunmap()函数解决这个问题， kmap函数一次只能映射一个物理页面。

