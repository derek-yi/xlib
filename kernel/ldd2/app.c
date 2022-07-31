#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define CMD_MAGIC   'k'
#define MEM_CMD1    _IO(CMD_MAGIC, 0x1a) // write
#define MEM_CMD2    _IO(CMD_MAGIC, 0x1b) // read

int main()
{
    int fd;
    int ret;
    int wdata, rdata;

    fd = open("/dev/memdev", O_RDWR);
    if( fd < 0 ) {
        printf("open memdev WRONG\n");
        return 0;
    }

    ret = ioctl(fd, MEM_CMD2, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);

    wdata = 42;
    ret = ioctl(fd, MEM_CMD1, &wdata);

    ret = ioctl(fd, MEM_CMD2, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
    
    close(fd);
    return 0;
}

