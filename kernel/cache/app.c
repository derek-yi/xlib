
#if 1
 
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

#define LENGTH 	(10*1024)

int main()
{
    int fd;
    int ret;
	IO_DATA_ST io_data;
    char mem_buff[6000];
	char *dyn_buff;
	char *huge_buff;
 
    fd = open("/dev/misc_op_cache", O_RDWR);
    if( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

	mem_buff[5900] = 0x5a;	
    io_data.buff = &mem_buff[0];
	io_data.len = sizeof(mem_buff);
    ret = ioctl(fd, 0x100, &io_data);
    if( ret < 0 ) {
        printf("ioctl failed \n");
        //return 0;
    }

	dyn_buff = (char *)malloc(LENGTH);
	if (dyn_buff) {
		dyn_buff[LENGTH-1] = 0x5a;
		io_data.buff = dyn_buff;
		io_data.len = LENGTH;
		ret = ioctl(fd, 0x100, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		free(dyn_buff);
	}

	huge_buff = (char *)mmap(0, LENGTH, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB), -1, 0);
	if (huge_buff != MAP_FAILED) {
		huge_buff[LENGTH-1] = 0x5a;
		io_data.buff = huge_buff;
		io_data.len = LENGTH;
		ret = ioctl(fd, 0x100, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		munmap(huge_buff, LENGTH);
	}
	
    close(fd);
    return 0;
}
 
#endif



