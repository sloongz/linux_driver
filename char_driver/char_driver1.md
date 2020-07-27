写一个简单的字符设备驱动， 然后不停的迭代添加功能来达到学习Linux driver的目的。

make生成char_driver1.ko文件

以root权限插入模块


```
insmod char_driver1.ko
```

查看设备

```
$cat /proc/devices
Character devices:
...
222 char_driver
...
```

创建一个设备结点

mknod /dev/x c 222 0

测试设备

```
# echo "123456789" > /dev/x
# cat /dev/x
123456789
```
dmesg log

```
[ 6990.257131] char enter
[ 7051.808456] x_open
[ 7051.808473] x_release
[ 7061.455665] x_open
[ 7061.455736] x_release
[ 7067.223526] char exit
```
如果静态分配主设备号需要考虑是否和在用的设备或已经定义的主设备号冲突

已经定义的主设备号查看kernel下的Documentation/devices.txt文档

在用的设备cat /proc/devices
