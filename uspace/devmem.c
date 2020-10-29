


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define MAP_SIZE        4096UL
#define MAP_MASK        (MAP_SIZE - 1)

/*
 * mem_addr: 
 * access_type: 1-byte, 2-short, 4-int
 */
unsigned long devmem_read(unsigned long mem_addr, int access_type) 
{
    int fd;
    void *map_base, *virt_addr;
    unsigned long read_result, writeval;
    off_t target = (off_t)mem_addr;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        return 0;
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if (map_base == (void *) -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
    switch(access_type) {
        case 1:
            read_result = *((unsigned char *) virt_addr);
            break;
        case 2:
            read_result = *((unsigned short *) virt_addr);
            break;
        case 4:
            read_result = *((unsigned long *) virt_addr);
            break;
        default:
            fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
            return 0; 
    }

    if (munmap(map_base, MAP_SIZE) == -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
    }
    close(fd);

    return read_result;
}


unsigned long devmem_write(unsigned long mem_addr, int access_type, unsigned long writeval) 
{
    int fd;
    void *map_base, *virt_addr;
    unsigned long read_result;
    off_t target = (off_t)mem_addr;

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)  {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        return 0;
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
    switch (access_type) {
        case 1:
            *((unsigned char *) virt_addr) = writeval;
            read_result = *((unsigned char *) virt_addr);
            break;
        case 2:
            *((unsigned short *) virt_addr) = writeval;
            read_result = *((unsigned short *) virt_addr);
            break;
        case 4:
            *((unsigned long *) virt_addr) = writeval;
            read_result = *((unsigned long *) virt_addr);
            break;
    }
    //printf("Written 0x%lu; readback 0x%lu\n", writeval, read_result);

    if(munmap(map_base, MAP_SIZE) == -1)  {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
    }
    close(fd);

    return 0;
}

int main()
{
    printf("devmem_read: 0x%lx \n", devmem_read(0x43ca0200, 4));
    
    return 0;
}

