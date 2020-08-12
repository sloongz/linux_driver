
- **proc**

linux /proc目录是一种虚拟文件系统， 存在于内存中， 用于调试以及用户和系统交互。

在proc目录下创建目录：

```
struct proc_dir_entry *proc_mkdir(const char *name, 
                                    struct proc_dir_entry *parent)
```
创建属性文件

```
struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
                    struct proc_dir_entry *parent,
                    const struct file_operations *proc_fops,
                    void *data)
```

如果不想管理复杂的内存， 可以使用seq_file管理文件

代码

https://github.com/sloongz/linux_driver/blob/master/proc/proc.c


编译 插入 创建结点

测试：
cat一下属性结点
```
$ cat /proc/test_dir/test_rw 


```
向属性结点写数据

```
$ echo "hello world" > /proc/test_dir/test_rw
```
再cat

```
$ cat /proc/test_dir/test_rw 
hello world


```
测试创建的proc目录可用
