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
