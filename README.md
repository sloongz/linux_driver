linux driver

[hello module](https://github.com/sloongz/linux_driver/tree/master/hello_module) 写一个简单的内核module

char_driver是简单的字符设备驱动

[char_driver1.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver1.c) 实现简单的创建字符设备、打开、关闭、读、写。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver1.md)。

[char_driver2.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver2.c) 增加insmod模块参数、动态分配主设备号、支持多个子设备。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver2.md)。

[char_driver3.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver3.c) 增加ioctl功能。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver3.md)。

[char_driver4.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver4.c) 增加lseek功能、增加mutex锁， [文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver4.md)。

[char_driver5.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver5.c) 增加spin_lock、semaphore、completion、atomic。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver5.md)。

[char_driver6.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver6.c) 在char_driver4.c的基础上增加wait_event。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver6.md)。

[char_driver7.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver7.c) 在char_driver6.c的基础上增加poll机制。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver7.md)。

[char_driver8.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver8.c) 在char_driver7.c的基础上增加信号机制。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver8.md)。

[char_driver9.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver9.c) 在char_driver7.c的基础上增加mmap接口。[文档](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver9.md)。

[proc.c](https://github.com/sloongz/linux_driver/blob/master/proc/proc.c) proc文件系统。[文档](https://github.com/sloongz/linux_driver/blob/master/proc/proc.md)。

[sys.c](https://github.com/sloongz/linux_driver/blob/master/sys/sys.c) sys文件系统。[文档](https://github.com/sloongz/linux_driver/blob/master/sys/sys.md)。

[debugfs.c](https://github.com/sloongz/linux_driver/blob/master/debugfs/debugfs.c) debugfs文件系统。[文档](https://github.com/sloongz/linux_driver/blob/master/debugfs/debugfs.md)。

[timer.c](https://github.com/sloongz/linux_driver/blob/master/timer/timer.c) timer定时器。[文档](https://github.com/sloongz/linux_driver/blob/master/timer/timer.md)。

[work_queue.c](https://github.com/sloongz/linux_driver/blob/master/work/work_queue.c) 工作队列。[文档](https://github.com/sloongz/linux_driver/blob/master/work/work_queue.md)。

[tasklet.c](https://github.com/sloongz/linux_driver/blob/master/tasklet/tasklet.c) tasklet软中断。[文档](https://github.com/sloongz/linux_driver/blob/master/tasklet/tasklet.md)

[kthread.c](https://github.com/sloongz/linux_driver/blob/master/kthread/kthread.c) 内核线程。[文档](https://github.com/sloongz/linux_driver/blob/master/kthread/kthread.md)。

[platform.c](https://github.com/sloongz/linux_driver/blob/master/platform/platform.c) 虚拟设备总线。[文档](https://github.com/sloongz/linux_driver/blob/master/platform/platform.md)

[misc.c](https://github.com/sloongz/linux_driver/blob/master/misc/platform.c) misc 设备驱动。[文档](https://github.com/sloongz/linux_driver/blob/master/misc/misc.md)

block_driver 是简单的块驱动

[blk_driver1.c](https://github.com/sloongz/linux_driver/blob/master/block_driver/blk_driver1.c)    使用系统默认IO调度器实现一个简单的内存设备文件。 [文档](https://github.com/sloongz/linux_driver/blob/master/block_driver/block_driver1.md)

[blk_driver2.c](https://github.com/sloongz/linux_driver/blob/master/block_driver/blk_driver2.c) 增加getgeo 函数，增加分区数，使用vmalloc申请内存 。[文档](https://github.com/sloongz/linux_driver/blob/master/block_driver/block_driver2.md)

[blk_driver3.c](https://github.com/sloongz/linux_driver/blob/master/block_driver/blk_driver3.c) 丢弃系统IO调度， 使用自己的“制造请求”函数。[文档](https://github.com/sloongz/linux_driver/blob/master/block_driver/block_driver3.md)

[simplefs.c](https://github.com/sloongz/linux_driver/blob/master/simplefs/simplefs.c) 实现简单的文件系统基本功能。[文档](https://github.com/sloongz/linux_driver/blob/master/simplefs/simplefs.md)