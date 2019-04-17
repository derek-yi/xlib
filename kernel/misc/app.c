
#if 1
 
#include <stdio.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
int main()
{
    int fd;
    int ret;
    int wdata, rdata;
 
    fd = open("/dev/miscdriver", O_RDWR);
    if( fd < 0 ) {
        printf("open miscdriver WRONG£¡\n");
        return 0;
    }
 
    ret = ioctl(fd, 0x101, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
 
    wdata = 42;
    ret = ioctl(fd, 0x100, &wdata);
 
    ret = ioctl(fd, 0x101, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
     
    close(fd);
    return 0;
}
 
#endif



