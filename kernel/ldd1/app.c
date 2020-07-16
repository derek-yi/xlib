
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
    char buf_read[4096]; // memdev设备的内容读入到该buf中

    if((fd = open("/dev/memdev",O_RDWR))==-1) // 打开memdev设备
        printf("open memdev WRONG！\n");
    else
        printf("open memdev SUCCESS!\n");

    printf("write: %s\n", buf); 

    write(fd, buf, sizeof(buf)); // 把buf中的内容写入memdev设备 
    lseek(fd, 0, SEEK_SET); // 把文件指针重新定位到文件开始的位置 
    read(fd, buf_read, sizeof(buf)); // 把memdev设备中的内容读入到buf_read中

    printf("read: %s\n",buf_read);
    close(fd);

    return 0;
}

