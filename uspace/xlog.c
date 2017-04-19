
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>


/*
初始化：
1、open文件
2、write 1M空数据到文件
3、mmap文件，ptr作为写指针
4、关闭文件

记录数据：
1、大于1M，另存文件
2、格式化数据，添加到ptr，长度递增

*/

#define XLOG_MAX_FILE_SIZE  1024*1024

char *xlog_write_ptr = NULL;
int xlog_write_size = 0;
int inner_id = 0;
char xlog_path_name[128] = "/home/derek";

int xlog_set_path_name(char *path_name)
{
    if(strlen(path_name) > 64 || path_name == NULL) return 1;

    sprintf(xlog_path_name, "%s", path_name);
        
    return 0;
}

int xlog_file_restore(int mid)
{
    int fd;
    char file_name[128];

    sprintf(file_name, "%s/xlog.%d.%d.txt", xlog_path_name, mid, inner_id++);
    
    fd = open(file_name, O_RDWR|O_CREAT, 0);
    if(fd < 0) {
        printf("open: %s\n", strerror(errno));
        return 1;
    }

    xlog_write_ptr = malloc(XLOG_MAX_FILE_SIZE);
    if (xlog_write_ptr == NULL) {
        printf("malloc: %s\n", strerror(errno));
        return 1;
    }

    memset(xlog_write_ptr, 0, XLOG_MAX_FILE_SIZE);
    write(fd, xlog_write_ptr, XLOG_MAX_FILE_SIZE);

    xlog_write_ptr = mmap(0, XLOG_MAX_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);  
    if (xlog_write_ptr == (char *)-1) {
        printf("mmap: %s\n", strerror(errno));
        return 1;
    }
    xlog_write_size = 0;

    close(fd);
    return 0;
}

int xlog_buffer(int mid, char *buffer)
{
    char log_info[128];
    int str_len = strlen(buffer);
    
    if ( (xlog_write_size + str_len > XLOG_MAX_FILE_SIZE) || (xlog_write_ptr == 0) )
    {
        if ( xlog_file_restore(mid) < 0) 
            return 1;
    }
    
    sprintf(xlog_write_ptr + xlog_write_size, "%s", buffer);
    xlog_write_size += str_len;
    
    return 0;
}

int main()
{
    int i;
    char log_info[128];
    
    for(i = 0; i < 1000; i++) {
        sprintf(log_info, "hello, %d\n", i);
        xlog_buffer(1, log_info);
        sleep(1);
    }
    
    return 0;
}

