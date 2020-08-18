随着字符设备种类和数量的增加，设备号越来越紧张，为此Linux系统提出misc设备模型以解决此问题。

所有misc设备其主设备号都是10，不同设备使用不同的次设备号区分。另外misc设备驱动会为设备自动创建设备文件，不需要想cdev设备那样，需要自己手动创建，所以使用起来更为方便。

misc设备驱动共享一个设备驱动号MISC_MAJOR。它在include\linux\major.h中定义：


```
#define MISC_MAJOR 10
```

miscdevice的结构体例如以下，它在include\linux\miscdevice.h中定义：

```
struct  miscdevice {
 int  minor;
 const  char  *name;
 const  struct file_operations  *fops;
 struct  list_head  list;
 struct  device  *parent;
 struct  device  *this_device;
 const  char  *nodename;
 mode_t  mode;
};
```
misc设备驱动的注冊和注销时用这两个函数，他们也定义在include\linux\miscdevice.h中：

```
extern int misc_register(struct miscdevice * misc);
extern int misc_deregister(struct miscdevice *misc);
```
再分配此设备号时，能够设为MISC_DYNAMIC_MINOR。这样会自己主动分配此设备号，如：

```
static struct miscdevice misc = {
 .minor = MISC_DYNAMIC_MINOR,
 .name = DEVICE_NAME,
 .fops = &dev_fops,
};
```
dev_fops 就是字符设备的文件夹操作结构

代码：

https://github.com/sloongz/linux_driver/blob/master/misc/platform.c

编译使用


```
$ sudo insmod platform.ko
```

```
$ cat /proc/misc 
 54 xmisc
 55 vsock
 56 vmci
235 autofs
 57 memory_bandwidth
 58 network_throughput
 59 network_latency
 60 cpu_dma_latency
227 mcelog
236 device-mapper
223 uinput
  1 psaux
200 tun
237 loop-control
 61 lightnvm
175 agpgart
183 hw_random
228 hpet
229 fuse
 62 ecryptfs
231 snapshot
242 rfkill
 63 vga_arbiter
```

```
$ ls -l /dev/xmisc 
crw------- 1 root root 10, 54 Aug 17 03:08 /dev/xmisc
```
看到misc设备的次设备号是54, 主设备号是10


读写操作和字符设备一样

```
$ sudo sh -c 'echo 1234567x > /dev/xmisc'
$ sudo cat /dev/xmisc
1234567x
$ sudo rmmod platform 
$ dmesg
[ 9889.632644] x_drv_probe
[ 9889.634510] x_init end
[ 9965.621853] x_open, dev:xmiscmajor:10 minor:54
[ 9965.621885] x_release
[ 9973.148865] x_open, dev:xmiscmajor:10 minor:54
[ 9973.148956] x_release
[ 9980.077937] x_drv_remove end
[ 9980.077968] x_exit end

```
