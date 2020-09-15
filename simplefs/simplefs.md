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
