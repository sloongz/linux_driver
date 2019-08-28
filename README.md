linux driver
笔记放在了[https://sloongz.github.io](https://sloongz.github.io)

[hello module](https://github.com/sloongz/linux_driver/tree/master/hello_module) 写一个简单的内核module

char_driver是简单的设备驱动

[char_driver1.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver1.c) 实现简单的创建字符设备、打开、关闭、读、写

[char_driver2.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver2.c) 增加insmod模块参数、动态分配主设备号、支持多个子设备

[char_driver3.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver3.c) 增加ioctl功能

[char_driver4.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver4.c) 增加lseek功能、增加mutex锁

[char_driver5.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver5.c) 增加spin_lock、semaphore、completion、atomic

[char_driver6.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver6.c) 在char_driver4.c的基础上增加wait_event

[char_driver7.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver7.c) 在char_driver6.c的基础上增加poll机制

[char_driver8.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver8.c) 在char_driver7.c的基础上增加信号机制

[char_driver9.c](https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver9.c) 在char_driver7.c的基础上增加mmap接口
