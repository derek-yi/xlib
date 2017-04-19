/******************************************************************************
*Name: memdevapp.c
*Desc: memdev字符设备驱动程序的测试程序。先往memedev设备写入内容，然后再
* 从该设备中把内容读出来。
*Parameter: 
*Return:
*Author: derek
*Date: 2013
********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <linux/fcntl.h>

int main()
{
    int fd;
    char buf[]="this is a example for character devices driver by derek!";
    char buf_read[4096]; //memdev设备的内容读入到该buf中

    if((fd = open("/dev/memdev",O_RDWR))==-1) //打开memdev设备
        printf("open memdev WRONG！\n");
    else
        printf("open memdev SUCCESS!\n");

    printf("write: %s\n", buf); 

    write(fd, buf, sizeof(buf)); //把buf中的内容写入memdev设备 
    lseek(fd, 0, SEEK_SET); //把文件指针重新定位到文件开始的位置 
    read(fd, buf_read, sizeof(buf)); //把memdev设备中的内容读入到buf_read中

    printf("read: %s\n",buf_read);
    close(fd);

    return 0;
}
