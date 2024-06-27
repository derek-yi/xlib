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

#define MEM_SIZE 			(6 * 4096)

// translate a virtual address to a physical one via /proc/self/pagemap
intptr_t virt_to_phys(void* virt) 
{
	long pagesize = sysconf(_SC_PAGESIZE);
	int fd = open("/proc/self/pagemap", O_RDONLY);
	intptr_t phy = 0;
    int ret;
    
	// pagemap is an array of pointers for each normal-sized page
	ret = lseek(fd, (intptr_t) virt / pagesize * sizeof(intptr_t), SEEK_SET);
    if (ret < 0) return 0;

	read(fd, &phy, sizeof(phy));
	close(fd);
    
	if (!phy) {
		printf("failed to translate virtual address %p to physical address \n", virt);
	}
    
	// bits 0-54 are the page number
	return (phy & 0x7fffffffffffffULL) * pagesize + ((intptr_t) virt) % pagesize;
}

int main()
{
    int fd;
    int ret;
	IO_DATA_ST io_data;
    char mem_buff[MEM_SIZE];
	char *dyn_buff;
	char *huge_buff;
	void *phy_addr;
 
    fd = open("/dev/misc_op_cache", O_RDWR);
    if( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

	phy_addr = virt_to_phys(mem_buff);
	printf("local va 0x%p, pa 0x%p \n", mem_buff, phy_addr);
    io_data.buff = phy_addr;
	io_data.len = sizeof(mem_buff);
    ret = ioctl(fd, 0x102, &io_data);
    if( ret < 0 ) {
        printf("ioctl failed \n");
        //return 0;
    }

	dyn_buff = (char *)malloc(MEM_SIZE);
	if (dyn_buff) {
		phy_addr = virt_to_phys(dyn_buff);
		printf("heap va 0x%p, pa 0x%p \n", dyn_buff, phy_addr);
		io_data.buff = phy_addr;
		io_data.len  = MEM_SIZE;
		ret = ioctl(fd, 0x102, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		free(dyn_buff);
	}

	huge_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB), -1, 0);
	if (huge_buff != MAP_FAILED) {
		phy_addr = virt_to_phys(huge_buff);
		printf("hugepage va 0x%p, pa 0x%p \n", huge_buff, phy_addr);
		io_data.buff = phy_addr;
		io_data.len  = MEM_SIZE;
		ret = ioctl(fd, 0x102, &io_data);
		if( ret < 0 ) {
			printf("ioctl failed \n");
			//return 0;
		}
		munmap(huge_buff, MEM_SIZE);
	}
	
    close(fd);
    return 0;
}

