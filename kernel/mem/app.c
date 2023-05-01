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

#define MEM_SIZE 	(6*1024)

int main()
{
    int fd;
    int ret;
	IO_DATA_ST io_data;
    char mem_buff[MEM_SIZE];
	char *dyn_buff;
	char *huge_buff;
 
    fd = open("/dev/misc_op_cache", O_RDWR);
    if( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

	mem_buff[MEM_SIZE-2] = 0x5a;	
    io_data.buff = &mem_buff[0];
	io_data.len  = sizeof(mem_buff);
    ret = ioctl(fd, 0x100, &io_data);
    if( ret < 0 ) {
        printf("ioctl failed \n");
        //return 0;
    }

	dyn_buff = (char *)malloc(MEM_SIZE);
	if (dyn_buff) {
		dyn_buff[MEM_SIZE-1] = 0x5a;
		io_data.buff = dyn_buff;
		io_data.len  = MEM_SIZE;
		ret = ioctl(fd, 0x100, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		free(dyn_buff);
	}

	huge_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB), -1, 0);
	if (huge_buff != MAP_FAILED) {
		huge_buff[MEM_SIZE-1] = 0x5a;
		io_data.buff = huge_buff;
		io_data.len  = MEM_SIZE;
		ret = ioctl(fd, 0x100, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		munmap(huge_buff, MEM_SIZE);
	}
	
    close(fd);
    return 0;
}


