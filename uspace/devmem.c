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

#define AT_BYTE         1
#define AT_SHORT        2
#define AT_WORD         4

int dbg_mode = 1;
int memdev_fd = -1;

int devmem_addr_valid(unsigned long mem_addr)
{
    //todo
    return 1;
}

/*************************************************************************
 * 物理地址读函数
 * mem_addr     - io设备地址
 * access_type  - 读取数据长度类型
 * return       - 读取的值
 *************************************************************************/
unsigned long devmem_read(unsigned long mem_addr, int access_type) 
{
    void *map_base, *virt_addr;
    unsigned long read_result;
    off_t target = (off_t)mem_addr;

    if (!devmem_addr_valid(mem_addr)) {
        if(dbg_mode) printf("%s@%d: invalid addr 0x%lx", __FUNCTION__, __LINE__, mem_addr);
        return 0;
    }

    if (memdev_fd < 0) {
        if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 ) {
            if(dbg_mode) printf("%s@%d: open failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
            return 0;
        }
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, target & ~MAP_MASK);
    if (map_base == (void *) -1) {
        if(dbg_mode) printf("%s@%d: mmap failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
    switch(access_type) {
        case AT_BYTE:
            read_result = *((unsigned char *) virt_addr);
            break;
        case AT_SHORT:
            read_result = *((unsigned short *) virt_addr);
            break;
        case AT_WORD:
            read_result = *((unsigned long *) virt_addr);
            break;
        default:
            if(dbg_mode) printf("%s@%d: access type %d", __FUNCTION__, __LINE__, access_type);
            return 0; 
    }

    if (munmap(map_base, MAP_SIZE) == -1) {
        if(dbg_mode) printf("%s@%d: munmap failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
    }

    return read_result;
}

/*************************************************************************
 * 物理地址写函数
 * mem_addr     - io设备地址
 * access_type  - 写入数据长度类型
 * writeval     - 写入的值
 *************************************************************************/
unsigned long devmem_write(unsigned long mem_addr, int access_type, unsigned long writeval) 
{
    void *map_base, *virt_addr;
    //unsigned long read_result;
    off_t target = (off_t)mem_addr;

    if (memdev_fd < 0) {
        if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 )  {
            if(dbg_mode) printf("%s@%d: open failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
            return 0;
        }
    }

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
        if(dbg_mode) printf("%s@%d: mmap failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
        return 0;
    }
    
    virt_addr = map_base + (target & MAP_MASK);
    switch (access_type) {
        case AT_BYTE:
            *((unsigned char *) virt_addr) = writeval;
            //read_result = *((unsigned char *) virt_addr);
            break;
        case AT_SHORT:
            *((unsigned short *) virt_addr) = writeval;
            //read_result = *((unsigned short *) virt_addr);
            break;
        case AT_WORD:
            *((unsigned long *) virt_addr) = writeval;
            //read_result = *((unsigned long *) virt_addr);
            break;
    }
    //printf("Written 0x%lu; readback 0x%lu\n", writeval, read_result);

    if (munmap(map_base, MAP_SIZE) == -1)  {
        if(dbg_mode) printf("%s@%d: munmap failed(%s)", __FUNCTION__, __LINE__, strerror(errno));
    }

    return 0;
}

int usage(char *main_cmd)
{
    printf("<%s> read <addr>\n", main_cmd);
    printf("<%s> write <addr> <value> \n", main_cmd);
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    unsigned long addr, value;

    if (argc < 2) {
        usage(argv[0]);
        return 0;
    }
 
    if (argv[1][0] == 'r') {
        addr = strtoul(argv[2], 0, 0);
        value = devmem_read(addr, AT_WORD);
        printf("[0x%08lx] = 0x%lx (%ld)\n", addr, value, value);
        return 0;
    }
  
    if (argc < 2) {
        usage(argv[0]);
        return 0;
    }
    
    addr = strtoul(argv[2], 0, 0);
    value = strtoul(argv[3], 0, 0);
    devmem_write(addr, AT_WORD, value);
    printf("[0x%08lx] = 0x%lx (%ld)\n", addr, value, value);
   
    return 0;
}
 


