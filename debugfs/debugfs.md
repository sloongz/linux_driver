
- **debugfs**

debugfs 是一种用于内核调试的虚拟文件系统，内核开发者通过debugfs和用户空间交换数据。

代码

https://github.com/sloongz/linux_driver/blob/master/debugfs/debugfs.c

使用debugfs需要内核打开选项

```
Kernel hacking —>
-*- Debug Filesystem
```
然后挂载

```
# mount -t debugfs none yourdir
```



测试：
挂载到了目录/home/ubuntu/fs下


```
/home/ubuntu/fs# ls test_debugfs/
test_rw

```
读文件

```
# cat test_debugfs/test_rw 


```
写文件

```
# echo "hello world" > test_debugfs/test_rw
```
读文件

```
# cat test_debugfs/test_rw 
hello world

```
测试debugfs接口可用。