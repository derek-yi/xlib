#include <stdio.h>  
#include <fcntl.h>  
#include <sys/mman.h>  
#include <stdlib.h>  
#include <string.h>  
  
int main(void)  
{  
    int fd, ret;  
    char *mapBuf;  
    int io_data = 100;
    
    fd = open("/dev/miscdriver", O_RDWR);
    if (fd < 0)  {  
        printf("open failed, fd %d \n",fd);  
        return -1;  
    }  
    
    mapBuf = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapBuf == NULL) {
        printf("mmap failed \n");
        close(fd);
        return 0;  
    }

    for (int i = 0; i < 5; i++) {
        printf("mapBuf[%d] %d \n", i, mapBuf[i]);
        mapBuf[i] += 1;
    }

    io_data = 0;
    ret = ioctl(fd, 0x101, &io_data);
    printf("ioctl ret %d, io_data %d \n", ret, io_data);

    io_data += 1;
    ret = ioctl(fd, 0x100, &io_data);
    printf("ioctl ret %d \n", ret);    
    
    munmap(mapBuf, 1024);
    close(fd);
    return 0;  
}  


