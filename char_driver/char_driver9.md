
- **mmap**

将设备地址映射到用户空间

驱动中mmap原型：

```
int(*mmap)(struct file *, struct vm_area_struct*);
```
用户空间原型：

```
void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
```

内存映射步骤：

1. 用open系统调用打开文件, 并返回描述符fd；
2. 用mmap建立内存映射, 并返回映射首地址指针start；
3. 对映射(文件)进行各种操作
4. 用munmap(void *start, size_t lenght)关闭内存映射
5. 用close系统调用关闭文件fd

函数：void *mmap(void *start,size_t length,int prot,int flags,int fd,off_t offsize); 

```
参数start：指向欲映射的内存起始地址，通常设为 NULL，
            代表让系统自动选定地址，映射成功后返回该地址。
参数length：代表将文件中多大的部分映射到内存。
参数prot：映射区域的保护方式。可以为以下几种方式的组合：
        PROT_EXEC 映射区域可被执行
        PROT_READ 映射区域可被读取
        PROT_WRITE 映射区域可被写入
        PROT_NONE 映射区域不能存取
参数flags：影响映射区域的各种特性。在调用mmap()时必须要指定MAP_SHARED           
            或MAP_PRIVATE。
        MAP_FIXED 如果参数start所指的地址无法成功建立映射时，则放弃映射，不对地
                    址做修正。通常不鼓励用此旗标。
        MAP_SHARED对映射区域的写入数据会复制回文件内，而且允许其他映射该文件的进
                    程共享。
        MAP_PRIVATE 对映射区域的写入操作会产生一个映射文件的复制，即私人的“写入
                    时复制”（copywrite）对此区域作的任何修改都不会写回原来的文件内容。
        MAP_ANONYMOUS建立匿名映射。此时会忽略参数fd，不涉及文件，而且映射区域无
                    法和其他进程共享
        MAP_DENYWRITE只允许对映射区域的写入操作，其他对文件直接写入的操作将会被
                    拒绝。
        MAP_LOCKED 将映射区域锁定住，这表示该区域不会被置换（swap）。
参数fd：要映射到内存中的文件描述符。如果使用匿名内存映射时，即flags中设置了
        MAP_ANONYMOUS，fd设为-1。有些系统不支持匿名内存映射，则可以使用fopen打开
        /dev/zero文件，然后对该文件进行映射，可以同样达到匿名内存映射的效果。
参数offset：文件映射的偏移量，通常设置为0，代表从文件最前方开始对应，offset必须
            是分页大小的整数倍。

```

内核使用remap_pfn_range 映射内核内存到用户空间
```
/**
 * remap_pfn_range - remap kernel memory to userspace
 * @vma: user vma to map to
 * @addr: target user address to start at
 * @pfn: physical address of kernel memory
 * @size: size of map area
 * @prot: page protection flags for this mapping
 *
 *  Note: this is only safe if the mm semaphore is held when called.
 */
int remap_pfn_range(struct vm_area_struct *vma, unsigned long addr,
            unsigned long pfn, unsigned long size, pgprot_t prot)

```

用户空间的mmap接口最终会调用驱动中的mmap接口

代码：
修改char_driver7.c 添加mmap函数 char_driver9.c 
https://github.com/sloongz/linux_driver/blob/master/char_driver/char_driver9.c

char9_test.c

```
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define DEV_NAME "/dev/x"

#define G_MEM_SIZE 1024

#define X_MAGIC   'X'
#define X_MAX_NR  5
#define X_IO_SET	_IO(X_MAGIC, 0)
#define X_IO_CLEAR	_IO(X_MAGIC, 1)
#define X_IOR		_IOR(X_MAGIC, 2, char[G_MEM_SIZE])
#define X_IOW		_IOW(X_MAGIC, 3, char[G_MEM_SIZE])
#define X_IOR_MAP	_IOR(X_MAGIC, 4, char[G_MEM_SIZE])

static int execute_cmd(const char *cmd, char *result)   
{   
    char buf_ps[1024];   
    char ps[1024]={0};   
    FILE *ptr;   
    strcpy(ps, cmd);   
    if ((ptr=popen(ps, "r"))!=NULL) {   
        while (fgets(buf_ps, 1024, ptr)!=NULL)   
        {   
            strcat(result, buf_ps);   
            if (strlen(result)>8192)   
              break;   
        }   
        pclose(ptr);   
        ptr = NULL;   
        return strlen(result);
    } else {   
        printf("popen %s error\n", ps);   
        return 0;
    }   
}

int main(int argv, char **argc)
{
	int fd;
	int ret;
	char *map_buf;
	pid_t pid;
	char buf[G_MEM_SIZE] = {0};
	char cmd[64] = {0};
	char result[8192] = {0};

	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		printf("open error\n");
		return 0;
	}
	pid = getpid();

	sprintf(cmd, "cat /proc/%d/maps", pid);
	execute_cmd(cmd, result);	
	printf("=====check vm mem 1========\n");
	printf("%s\n", result);
	printf("=======================\n");

	map_buf = mmap(NULL, G_MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_buf < 0) {
		printf("map fail\n");
	}
	printf("%s\n", strcpy(map_buf, "hello world"));

	ret = ioctl(fd, X_IOR_MAP, buf);
	if (ret < 0) 
		printf("ioctl X_IOR fail\n");
	else
		printf("read data form kernel: %s\n", buf);

	memset(result, 0, sizeof(result));
	sprintf(cmd, "cat /proc/%d/maps", pid);
	execute_cmd(cmd, result);
	printf("\n\n=====check vm mem 2========\n");
	printf("%s\n", result);
	printf("=======================\n");

	munmap(map_buf, G_MEM_SIZE);
	close(fd);

	return 0;
}

```
编译驱动， 插入ko，创建设备结点

运行测试代码

```
# ./char9_test
=====check vm mem 1========
00400000-00401000 r-xp 00000000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
00601000-00602000 r--p 00001000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
00602000-00603000 rw-p 00002000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
007b0000-007d1000 rw-p 00000000 00:00 0                                  [heap]
7f5fdf957000-7f5fdfb17000 r-xp 00000000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfb17000-7f5fdfd17000 ---p 001c0000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd17000-7f5fdfd1b000 r--p 001c0000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd1b000-7f5fdfd1d000 rw-p 001c4000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd1d000-7f5fdfd21000 rw-p 00000000 00:00 0 
7f5fdfd21000-7f5fdfd47000 r-xp 00000000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff2c000-7f5fdff2f000 rw-p 00000000 00:00 0 
7f5fdff46000-7f5fdff47000 r--p 00025000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff47000-7f5fdff48000 rw-p 00026000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff48000-7f5fdff49000 rw-p 00000000 00:00 0 
7ffe7d125000-7ffe7d146000 rw-p 00000000 00:00 0                          [stack]
7ffe7d179000-7ffe7d17c000 r--p 00000000 00:00 0                          [vvar]
7ffe7d17c000-7ffe7d17e000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

=======================
hello world
read data form kernel: hello world


=====check vm mem 2========
00400000-00401000 r-xp 00000000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
00601000-00602000 r--p 00001000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
00602000-00603000 rw-p 00002000 00:30 1749                               /mnt/hgfs/vmshare/linux_driver/simple_driver/app/test8
007b0000-007d1000 rw-p 00000000 00:00 0                                  [heap]
7f5fdf957000-7f5fdfb17000 r-xp 00000000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfb17000-7f5fdfd17000 ---p 001c0000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd17000-7f5fdfd1b000 r--p 001c0000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd1b000-7f5fdfd1d000 rw-p 001c4000 08:01 5781233                    /lib/x86_64-linux-gnu/libc-2.23.so
7f5fdfd1d000-7f5fdfd21000 rw-p 00000000 00:00 0 
7f5fdfd21000-7f5fdfd47000 r-xp 00000000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff2c000-7f5fdff2f000 rw-p 00000000 00:00 0 
7f5fdff45000-7f5fdff46000 rw-s 00000000 00:06 445                        /dev/x
7f5fdff46000-7f5fdff47000 r--p 00025000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff47000-7f5fdff48000 rw-p 00026000 08:01 5781231                    /lib/x86_64-linux-gnu/ld-2.23.so
7f5fdff48000-7f5fdff49000 rw-p 00000000 00:00 0 
7ffe7d125000-7ffe7d146000 rw-p 00000000 00:00 0                          [stack]
7ffe7d179000-7ffe7d17c000 r--p 00000000 00:00 0                          [vvar]
7ffe7d17c000-7ffe7d17e000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

```
看到设备map一段内存， 向内存中写入“hello world” ， 也可以从内存中读取，

证明映射成功。

通过命令 cat /proc/进程pid/maps可以看到设备映射的内存


```
7f5fdff45000-7f5fdff46000 rw-s 00000000 00:06 445                        /dev/x
```


```
vm_start： 7f5fdff45000     此段虚拟地址空间起始地址
vm_end：   7f5fdff46000     此段虚拟地址空间结束地址
vm_flags：  rw-s    此段虚拟地址空间的属性。每种属性用一个字段表示，r表示可读，
                    w表示可写，x表示可执行，p和s共用一个字段，互斥关系，p表示私
                    有段，s表示共享段，如果没有相应权限，则用’-’代替
vm_pgoff：00000000  对有名映射，表示此段虚拟内存起始地址在文件中以页为单位的偏
                    移
vm_file->f_dentry->d_inode->i_sb->s_dev： 00:06   映射文件所属设备号
vm_file->f_dentry->d_inode->i_ino：   445     映射文件所属节点号。
映射文件名：  /dev/x

```
