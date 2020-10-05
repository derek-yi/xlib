
#if 0
 
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
        printf("open miscdriver WRONG！\n");
        return 0;
    }
 
    ret = ioctl(fd, 0x101, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
 
    wdata = rdata + 10;
    ret = ioctl(fd, 0x100, &wdata);
 
    ret = ioctl(fd, 0x101, &rdata);
    printf("ioctl: ret=%d rdata=%d\n", ret, rdata);
     
    close(fd);
    return 0;
}
 
#endif

#include <stdio.h>  
#include <fcntl.h>  
#include <sys/mman.h>  
#include <stdlib.h>  
#include <string.h>  
  
int main( void )  
{  
    int fd;  
    char *buffer;  
    char *mapBuf;  
    
    fd = open("/dev/miscdriver", O_RDWR);//打开设备文件，内核就能获取设备文件的索引节点，填充inode结构  
    if(fd<0)  
    {  
        printf("open device is error,fd = %d\n",fd);  
        return -1;  
    }  
    
    /*测试一：查看内存映射段*/  
    printf("before mmap, my pid: %d\n", getpid());  
    sleep(15);//睡眠15秒，查看映射前的内存图cat /proc/pid/maps  
    buffer = (char *)malloc(1024);  
    memset(buffer, 0, 1024);  
    mapBuf = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);//内存映射，会调用驱动的mmap函数  
    printf("after mmap\n");  
    sleep(15);//睡眠15秒，在命令行查看映射后的内存图，如果多出了映射段，说明映射成功  
      
    /*测试二：往映射段读写数据，看是否成功*/  
    strcpy(mapBuf, "Driver Test");//向映射段写数据  
    memset(buffer, 0, 1024);  
    strcpy(buffer, mapBuf);//从映射段读取数据  
    printf("buf = %s\n", buffer);//如果读取出来的数据和写入的数据一致，说明映射段的确成功了  
      
      
    munmap(mapBuf, 1024);//去除映射  
    free(buffer);  
    close(fd);//关闭文件，最终调用驱动的close  
    return 0;  
}  


