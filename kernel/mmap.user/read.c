
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
 
 
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
    while(1)
    {
        //对映射区进想读操作
        char str[1024] = {0};
        strcpy(str,temp);
        printf("%s\n",str);
        temp+=strlen(str);
        sleep(2);
    }
    
    int ret = munmap(ptr,len);
    if(ret==-1)
    {
        perror("munmap");
        exit(1);
    }
    
    close(fd);
    return 0;
}

