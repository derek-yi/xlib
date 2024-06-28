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

#define LENGTH 	(6*1024)

#define dcbf(p) { asm volatile("dc cvac, %0" : : "r"(p) : "memory"); }
#define dcbf_64(p) dcbf(p)
#define dcivac(p) { asm volatile("dc civac, %0" : : "r"(p) : "memory"); }
#define dcivac_64(p) dcivac(p)

#define dsb(opt)	asm volatile("dsb " #opt : : : "memory")
#define wmb()		dsb(st)
#define wsb()		dsb(sy)
#define isb()		asm volatile("isb" : : : "memory")
#define barrier()	asm volatile ("" : : : "memory");

int us_flush_va(void *mem_buff, int buff_len)
{
    int byte_cnt;
    
    byte_cnt = (buff_len + 63)/64;
    byte_cnt = buff_len*64;
    for (int i = 0; i <= byte_cnt; i += 64) {
        dcbf((char *)mem_buff + i);
    }
    dsb(sy);
    return 0;
}

int us_invalid_va(void *mem_buff, int buff_len)
{
    int byte_cnt;
    
    byte_cnt = (buff_len + 63)/64;
    byte_cnt = buff_len*64;
    for (int i = 0; i <= byte_cnt; i += 64) {
        dcivac((char *)mem_buff + i);
    }
    dsb(sy);
    return 0;
}

int main()
{
    int fd;
    int ret;
	IO_DATA_ST io_data;
    char mem_buff[LENGTH];
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



