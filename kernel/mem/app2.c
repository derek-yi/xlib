#include <stdio.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef struct
{
	void *buff;
	unsigned long len;
}IO_DATA_ST;

#define DMA_BUFF_SZ 			(16 * 4096)

int main(int argc, char **argv)
{
    int fd;
    int ret;
	int mem_type = 0;
	unsigned long phy_addr = 0;
    char *map_mem;

	if (argc < 2) {
		printf("usage: %s <mem_type> \n", argv[0]);
		return 0;
	}

	mem_type = atoi(argv[1]);
    fd = open("/dev/misc_cache", O_RDWR);
    if( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

    ret = ioctl(fd, 0x300, &mem_type);
    if( ret < 0 ) {
        printf("ioctl failed \n");
        goto CLOSE_EXIT;
    }

	map_mem = (char *)mmap(NULL, DMA_BUFF_SZ, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
	if (map_mem != MAP_FAILED) {
		map_mem[0] = 0x100 + mem_type;
		ret = ioctl(fd, 0x301, &phy_addr);
		if ( ret < 0 ) {
			printf("ioctl failed \n");
		} 
		printf("map_map 0x%p, phy_addr 0x%lx \n", map_mem, phy_addr);
		printf("check 0x%x \n", map_mem[0]);
		munmap(map_mem, DMA_BUFF_SZ);
	}

CLOSE_EXIT:
    close(fd);
    return 0;
}

