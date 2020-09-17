##### 1 建立一个文件系统， 可以挂载
API：

```
int register_filesystem(struct file_system_type * fs)
int unregister_filesystem(struct file_system_type * fs)
```
创建和删除文件系统

把新的文件系统类型注册到内核

```
struct file_system_type
```
file_system_type 要有名字，如果挂载，如何卸载的操作方法。

挂载使用mount_bdev函数

```
struct dentry *mount_bdev(struct file_system_type *fs_type,
    int flags, const char *dev_name, void *data,
    int (*fill_super)(struct super_block *, void *, int))
```
新的文件系统需要实现fill_super函数来处理新的super block， 并创建indoe、dentry返回 s_root。

测试：


dd 一个文件：
```
dd bs=4096 count=100 if=/dev/zero of=image
```
创建mount 目录：
```
mkdir mount_point

ls -li
1775427 drwxrwxr-x 2 sean sean   4096 Sep 13 20:43 mount_point
查看inode number  1775427
```
加载文件系统：
```
sudo insmod simple.ko
```
挂载文件系统：
```
sudo mount -o loop -t simplefs image ./mount_point
敲mount命令可以看到已经挂载上了
/home/sean/linux_driver/simplefs/image on /home/sean/linux_driver/simplefs/mount_point type simplefs (rw,relatime)

ls -li
32653 d--------- 1 root root      0 Sep 13 23:39 mount_point
查看inode number  32653
```
卸载
```
sudo umount ./mount_point
```
查看log
```
[  132.788109] simplefs enter
[  132.788114] successfully register simplefs
[  134.414451] i_ino:32653 mode:0x4000
[  134.414455] S_IFDIR:0x4000
[  134.414457] successfully mount /dev/loop0
[  155.009398] simplefs superblock is destroyed. Unmount succesful.

目录对应的inode number 是32653
```


```
    1 00000000: 01 00 00 00 78 56 34 12 00 10 00 00 00 00 00 00  ....xV4.........
    2 00000010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    3 00000020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    4 00000030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................

```

##### 2 构造一个正常的文件系统带super block\inode table\data block

```
/*
 * superblock 占用一个block-------------------------(block 0)
 *      version=1 
 *      magic=0x12345678 
 *      inodes_count=2  (root_inode and welcome file inode) 
 * =============================================================  
 *
 * 相当于 inode table, 占用一个block ---------------(block 1)
 * root_inode
 *     inode_no = 1
 *     data_block_number = 2 
 *     dir_children_count = 1
 * welcome file inode
 *     inode_no = 2
 *     data_block_number = 3
 * ============================================================
 *
 * 接下来是data block 
 *
 * root inode 对应的 dir 占用一个block ------------(block 2)
 *    写入文件名和文件对应的inode号 
 *    [file name | inode number]
 *    [file name | inode number]
 *    ...
 *
 * welcome file 文件内容占用一个block -------------(block 3)
 *    文件内容
 *
*/
```

##### 3 实现ls命令
ls 命令需要实现两个接口

```
1. VFS的readdir接口
struct file_operations
    int (*iterate) (struct file *, struct dir_context *);
    //填dir对应的inode和name
    
2. inode 的lookup接口
struct inode_operations
    struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
    //dentry add to queue 
```

测试
```
sean@ubuntu:~/linux_driver/simplefs/mount_point$ ls
hello
```
查看log

```
[  555.546089] simplefs enter
[  555.546106] successfully register simplefs
[  559.662261] The magic number obtained in disk is: [0x12345678]
[  559.662263] version: 1
[  559.662264] block size: 4096
[  559.662264] free block: 8
[  559.662571] i_ino:1 mode:0x4000
[  559.662575] successfully mount /dev/loop0
[  567.726795] simplefs_iterate() start
[  567.726797] file inode:1
[  567.726798] inode_no:1, data_block_number:2
[  567.726804] dir count;1
[  567.726806] filename:hello inode:2
[  567.726807] simplefs_iterate() end
[  567.726814] simplefs_lookup() start
[  567.726816] parent_inode:1, child file name:hello
[  567.726817] dir->inode_no:2 dir->filename:hello
[  567.726818] dir count:1
[  567.726818] find file:hello
[  567.726821] simplefs_lookup() end
[  567.726826] simplefs_iterate() start
[  567.726827] file inode:1
[  676.488078] simplefs superblock is destroyed. Unmount succesful.
```

##### 3 实现read命令
实现struct file_operations中的read函数，
把datablock中的数据通过 copy_to_user 传到用户空间。

测试
```
sean@ubuntu:~/linux_driver/simplefs/mount_point$ sudo cat hello
hello world, this is a test file store in data block
```

