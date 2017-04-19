
#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

#define IO_CMD_LEN      1024  

typedef struct 
{
    int file_id; //todo
    int rw_pos;  //todo
    int cmd_type;
    int buff_len;
    char io_buff[IO_CMD_LEN - 16];
}KLOG_CMD_INFO;

KLOG_CMD_INFO cmd_info;

int main()
{
    int fd;
    int ret = 0;

    fd = open("/dev/klog", O_RDWR);
    if( fd < 0 ) {
        printf("open memdev WRONG£¡\n");
        return 0;
    }

    sprintf(cmd_info.io_buff, "/home/derek/klog.txt");
    cmd_info.cmd_type = 1;
    ret = ioctl(fd, 0, &cmd_info);
    printf("open: ret=%d rdata:%s\n", ret, cmd_info.io_buff);

    for(int i = 0; i < 10; i++) {
        sprintf(cmd_info.io_buff, "%d: ==============================\n", i);
        cmd_info.buff_len = strlen(cmd_info.io_buff);
        cmd_info.cmd_type = 2;
        ret = ioctl(fd, 0, &cmd_info);
        printf("read: ret=%d rdata:%s\n", ret, cmd_info.io_buff);
    }

    cmd_info.cmd_type = 3;
    ret = ioctl(fd, 0, &cmd_info);
    printf("close: ret=%d rdata:%s\n", ret, cmd_info.io_buff);
   
    close(fd);
    return 0;
}

