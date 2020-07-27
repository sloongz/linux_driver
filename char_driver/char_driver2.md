改进一下char_driver2.c

- 增加insmod模块参数
- 动态分配主设备号
- 支持多个子设备

make 生成char_driver2.ko

定义了模块的参数

```
module_param(g_major, int, S_IRUGO);
```
int 类型， 变量g_major

程序中如果g_major为0的话就动态注册字符设备， 所以插入模块的是后g_major传参0
以root权限插入模块

```
# insmod char_driver2.ko g_major=0
```
查看设备

```
$cat /proc/devices
Character devices:
...
243 char_driver
...
```
创建设备结点

```
# mknod /dev/x c 243 0
# mknod /dev/x1 c 243 1
# mknod /dev/x2 c 243 2
# mknod /dev/x3 c 243 3
# mknod /dev/x4 c 243 4
# mknod /dev/x5 c 243 5

```
测试设备

```
# echo "0" > /dev/x
# echo "1" > /dev/x1
# echo "2" > /dev/x2
# echo "3" > /dev/x3
# echo "4" > /dev/x4
# cat /dev/x
0
# cat /dev/x1
1
# cat /dev/x2
2
# cat /dev/x3
3
# cat /dev/x4
4

```
每个子设备都可以单独操作。

刚才mknod了6个设备， 测试一下第6个设备

```
# echo "5" > /dev/x5
bash: /dev/x5: No such device or address

```
失败， 是因为程序中只初始化了5个字符设备， 无法绑定第6个了。




