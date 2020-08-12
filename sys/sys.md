
- **sys**

sysfs 虚拟文件系统提供了一种比 proc 更为理想的访问内核数据的途径，

而将 proc 保留给纯净的“进程文件系统”。

sys支持两种属性文件
1. 普通文本文件
2. 二进制文件

对应的数据结构如下：
```
struct attribute_group {
    const char      *name;
    umode_t         (*is_visible)(struct kobject *,
                          struct attribute *, int);
    struct attribute    **attrs;
    struct bin_attribute    **bin_attrs;
};

struct bin_attribute {
    struct attribute    attr;
    size_t          size;
    void            *private;
    ssize_t (*read)(struct file *, struct kobject *, struct bin_attribute *,
            char *, loff_t, size_t);
    ssize_t (*write)(struct file *, struct kobject *, struct bin_attribute *,
             char *, loff_t, size_t);
    int (*mmap)(struct file *, struct kobject *, struct bin_attribute *attr,
            struct vm_area_struct *vma);
};

struct kobj_attribute {
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,
            char *buf);
    ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,
             const char *buf, size_t count);
};
```
sys文件系统 内核中在 show/store 

函数参数上的buf/count参数已经是内核区的地址，可以直接操作。


创建了两个sys属性结点，普通文本和二进制的接口， 为了简单程序都写成了字符串了。

https://github.com/sloongz/linux_driver/blob/master/sys/sys.c

测试

```
# echo "hello world" > /sys/test_sysfs/xxx 
# cat /sys/test_sysfs/xxx 
hello world

# echo "hello world" > /sys/test_sysfs/bbb
# cat /sys/test_sysfs/bbb 
hello world

```
两个接口都可以使用。

参考：[https://www.ibm.com/developerworks/cn/linux/l-cn-sysfs/](https://www.ibm.com/developerworks/cn/linux/l-cn-sysfs/)