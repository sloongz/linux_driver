linux driver

[hello module](https://github.com/sloongz/linux_driver/tree/master/hello_module) 写一个简单的内核module

char_driver是简单的设备驱动

[char_driver1.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver1.c) 实现简单的创建字符设备、打开、关闭、读、写。[文档](https://sloongz.github.io/2019/08/25/char%20driver（一）/)。

[char_driver2.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver2.c) 增加insmod模块参数、动态分配主设备号、支持多个子设备。[文档](https://sloongz.github.io/2019/08/25/char%20driver（二）/)。

[char_driver3.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver3.c) 增加ioctl功能。[文档](https://sloongz.github.io/2019/08/25/char%20driver（三）/)。

[char_driver4.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver4.c) 增加lseek功能、增加mutex锁， [文档](https://sloongz.github.io/2019/08/25/char%20driver（四）/)。

[char_driver5.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver5.c) 增加spin_lock、semaphore、completion、atomic。[文档](https://sloongz.github.io/2019/08/25/char%20driver（五）/)。

[char_driver6.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver6.c) 在char_driver4.c的基础上增加wait_event。[文档](https://sloongz.github.io/2019/08/25/char%20driver（六）/)。

[char_driver7.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver7.c) 在char_driver6.c的基础上增加poll机制。[文档](https://sloongz.github.io/2019/08/25/char%20driver（七）/)。

[char_driver8.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver8.c) 在char_driver7.c的基础上增加信号机制。[文档](https://sloongz.github.io/2019/08/25/char%20driver（八）/)。

[char_driver9.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver9.c) 在char_driver7.c的基础上增加mmap接口。[文档](https://sloongz.github.io/2019/08/25/char%20driver（九）/)。

[proc.c](https://github.com/sloongz/linux_driver/blob/master/proc/proc.c) proc文件系统。[文档](https://sloongz.github.io/2019/10/10/proc（十）/)。

[sys.c](https://github.com/sloongz/linux_driver/blob/master/sys/sys.c) sys文件系统。[文档](https://sloongz.github.io/2019/10/10/sys（十一）/)。

[debugfs.c](https://github.com/sloongz/linux_driver/blob/master/debugfs/debugfs.c) debugfs文件系统。[文档](https://sloongz.github.io/2019/10/10/debugfs（十二）/)。

[timer.c](https://github.com/sloongz/linux_driver/blob/master/timer/timer.c) timer定时器。[文档](https://sloongz.github.io/2019/10/10/timer（十三）/)。

[work_queue.c](https://github.com/sloongz/linux_driver/blob/master/work/work_queue.c) 工作队列。[文档](https://sloongz.github.io/2019/10/10/work_queue（十四）/)。

[kthread.c](https://github.com/sloongz/linux_driver/blob/master/kthread/kthread.c) 内核线程。[文档](https://sloongz.github.io/2019/10/21/kthread（十五）/)。

[platform.c](https://github.com/sloongz/linux_driver/blob/master/platform/platform.c) 虚拟设备总线。[文档](https://sloongz.github.io/2019/10/21/platform（十六）/)
