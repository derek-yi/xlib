
#if 1
 
#include <stdio.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>

int usage(char *main_cmd)
{
    printf("<%s> read \n", main_cmd);
    printf("<%s> write <value> \n", main_cmd);
    return 0;
}

int main(int argc, char **argv)
{
    int fd = 0;
    int ret;
    int wdata, rdata;

    if (argc < 2) {
        usage(argv[0]);
        return 0;
    }
 
    fd = open("/dev/miscdriver", O_RDWR);
    if( fd < 0 ) {
        printf("open miscdriver WRONG\n");
        return 0;
    }

    if (argv[1][0] == 'r') {
        ret = ioctl(fd, 0x101, &rdata);
        printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
        goto clean_exit;
    }
  
    if (argc < 2) {
        usage(argv[0]);
        goto clean_exit;
    }
    
    wdata = atoi(argv[2]);
    ret = ioctl(fd, 0x100, &wdata);
    printf("ioctl: ret=%d wdata=%d\n", ret, wdata);
   
clean_exit:     
    if (fd > 0) close(fd);
    return 0;
}
 
#endif



