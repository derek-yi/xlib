#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
 
 
int main(void)
{
    int fd = open("temp",O_RDWR|O_CREAT,0664);
    if(fd==-1)
    {
        perror("open hello");
        exit(1);
    }
    
    ftruncate(fd,4096);
    int len = lseek(fd,0,SEEK_END);
    
    void* ptr = mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(ptr == MAP_FAILED)
    {
        perror("mmap error");
        exit(1);
    }
    
    char* temp = (char*)ptr;
    int i = 1;
    while(1)
    {
        //对映射区进行写操作
        char str[1024]={0};
        sprintf(str,"This is line %d",i);
        strcpy(temp,str);
        //temp[0] = 'a';
        printf("写入数据:%s\n",temp);
        temp += strlen(str);
        sleep(1);
        i++;
    }
    
    //释放内存映射区
    int ret = munmap(ptr,len);
    if(ret==-1)
    {
        perror("munmap");
        exit(1);
    }
    
    close(fd);
    return 0;
}


