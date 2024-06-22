#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
int main()
{
    unsigned char * map_base;
    unsigned long addr;
    unsigned char content;
    FILE *f;
    int n, fd;
	int map_size = 1024;
 
    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd == -1)
    {
        return (-1);
    }
 
    map_base = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x20000);
    if (map_base == 0)
    {
        printf("NULL pointer!\n");
    }
    else
    {
        printf("Successfull!\n");
    }
 
    for (int i = 0;i < map_size; ++i)
    {
        addr = (unsigned long)(map_base + i);
        content = map_base[i];
        printf("read address: 0x%lx \t content 0x%x \n", addr, (unsigned int)content);
 
        map_base[i] = (unsigned char)i;
        content = map_base[i];
        printf("updated address: 0x%lx \t content 0x%x \n", addr, (unsigned int)content);
    }
 
    close(fd);
    munmap(map_base, map_size);
 
    return (1);
}

