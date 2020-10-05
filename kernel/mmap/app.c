
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
        printf("open miscdriver WRONG��\n");
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
    
    fd = open("/dev/miscdriver", O_RDWR);//���豸�ļ����ں˾��ܻ�ȡ�豸�ļ��������ڵ㣬���inode�ṹ  
    if(fd<0)  
    {  
        printf("open device is error,fd = %d\n",fd);  
        return -1;  
    }  
    
    /*����һ���鿴�ڴ�ӳ���*/  
    printf("before mmap, my pid: %d\n", getpid());  
    sleep(15);//˯��15�룬�鿴ӳ��ǰ���ڴ�ͼcat /proc/pid/maps  
    buffer = (char *)malloc(1024);  
    memset(buffer, 0, 1024);  
    mapBuf = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);//�ڴ�ӳ�䣬�����������mmap����  
    printf("after mmap\n");  
    sleep(15);//˯��15�룬�������в鿴ӳ�����ڴ�ͼ����������ӳ��Σ�˵��ӳ��ɹ�  
      
    /*���Զ�����ӳ��ζ�д���ݣ����Ƿ�ɹ�*/  
    strcpy(mapBuf, "Driver Test");//��ӳ���д����  
    memset(buffer, 0, 1024);  
    strcpy(buffer, mapBuf);//��ӳ��ζ�ȡ����  
    printf("buf = %s\n", buffer);//�����ȡ���������ݺ�д�������һ�£�˵��ӳ��ε�ȷ�ɹ���  
      
      
    munmap(mapBuf, 1024);//ȥ��ӳ��  
    free(buffer);  
    close(fd);//�ر��ļ������յ���������close  
    return 0;  
}  


