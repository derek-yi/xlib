#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define CMD_MAGIC   'k'
#define MEM_CMD1    _IO(CMD_MAGIC, 0x1a) // init
#define MEM_CMD2    _IO(CMD_MAGIC, 0x1b) // write
#define MEM_CMD3    _IO(CMD_MAGIC, 0x1c) // read

typedef struct tagUSER_MSG_INFO
{
    int  uid;
    int  param1;
    int  param2;
    int  param3;
    char *msg;
}USER_MSG_INFO;


#define _APP_001_

#ifdef _APP_001_
int main()
{
    int fd;
    int ret;
    char buff[256];
    USER_MSG_INFO usr_msg;

    fd = open("/dev/kpipe", O_RDWR);
    if( fd < 0 ) {
        printf("open kpipe WRONG£¡\n");
        return 0;
    }

    usr_msg.uid = 0;
    usr_msg.param1 = 64; // max msg
    usr_msg.param2 = 128; // max len
    
    ret = ioctl(fd, MEM_CMD1, &usr_msg);
    printf("ioctl: ret=%d\n", ret);

    usr_msg.msg = &buff[0];
    sprintf(buff, "hello,pipe\n");
    ret = ioctl(fd, MEM_CMD2, &usr_msg);
    printf("ioctl: ret=%d\n", ret);

    usr_msg.msg = &buff[0];
    memset(buff, 0, 256);
    ret = ioctl(fd, MEM_CMD3, &usr_msg);
    printf("ioctl: ret=%d rdata=%s\n", ret, buff);
    
    close(fd);
    return 0;
}

#endif


#ifdef _APP_002_


#endif
