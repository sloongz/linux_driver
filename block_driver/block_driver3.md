- 丢弃系统IO调度， 使用自己的“制造请求”函数


```
void blk_queue_make_request(struct request_queue *q, make_request_fn *mfn)
```
对应一些设备并不需要进行复杂的IO调度，这时候， 可以直接“踢开”IO调度器，使用如下函数来绑定请求队列和“制造请求”函数（make_request_fn）。

blk_queue_make_request() 需要处理bio中的每个bio_vec

使用bio_for_each_segment(bvec, bio, iter) 在一个循环中搞定 bio 中的每个 bio_vec。

编译代码
https://github.com/sloongz/linux_driver/blob/master/block_driver/blk_driver3.c

使用：
```
$ sudo insmod blk_driver3.ko
```

```
$ sudo mkfs.ext4 /dev/mblk_dev
mke2fs 1.42.13 (17-May-2015)
Creating filesystem with 16384 1k blocks and 4096 inodes
Filesystem UUID: f496daf7-b907-4948-b421-646fd3b5d888
Superblock backups stored on blocks: 
	8193

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (1024 blocks): done

```

```
$ sudo mount /dev/mblk_dev AAA
```

```
$ cd AAA/
$ sudo sh -c 'echo xxxx > 1.txt'
AAA$ cat 1.txt 
xxxx
$ sudo rm 1.txt
$ ls
lost+found
```

查看调度算法：

```
$ cat /sys/block/mblk_dev/queue/scheduler 
none
```


```
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/mblk_dev    15M  140K   14M   2% /home/sean/github/linux_driver/block_driver/AAA

```

```
$ sudo umount AAA
$ sudo rmmod blk_driver3
```
