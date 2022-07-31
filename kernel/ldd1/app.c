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
    char buf[]= "this is a example for character devices driver by derek!";
    char buf_read[4096];

    if((fd = open("/dev/memdev", O_RDWR))==-1)
        printf("open memdev WRONG\n");
    else
        printf("open memdev SUCCESS!\n");

    printf("write: %s\n", buf); 

    write(fd, buf, sizeof(buf));
    lseek(fd, 0, SEEK_SET);
    read(fd, buf_read, sizeof(buf));

    printf("read: %s\n",buf_read);
    close(fd);

    return 0;
}

