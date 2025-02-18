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
#include <stddef.h>

#define MEM_SIZE 	(16*1024)

// translate a virtual address to a physical one via /proc/self/pagemap
intptr_t virt_to_phys(void* virt) 
{
	long pagesize = getpagesize();
	//long pagesize = sysconf(_SC_PAGESIZE);
	int fd = open("/proc/self/pagemap", O_RDONLY);
	intptr_t phy = 0;
    int ret;

	if (fd < 0) {
		printf("open failed \n");
		return 0;
	}
	
	// pagemap is an array of pointers for each normal-sized page
	ret = lseek(fd, ((intptr_t) virt / pagesize) * sizeof(intptr_t), SEEK_SET);
    if (ret == -1) {
		printf("lseek failed \n");
		return 0;
	}

	read(fd, &phy, sizeof(phy));
	close(fd);
    
	if (!phy) {
		printf("failed to get virtual address %p \n", virt);
	}

	// bits 0-54 are the page number
	return (phy & 0x7fffffffffffffULL) * pagesize + ((intptr_t) virt) % pagesize;
}


int alloc_huge_page(int index)
{
    char file_name[128];
    int fd;
    void *addr;

    sprintf(file_name, "/mnt/huge/hugepage%d", index);
    fd = open(file_name, O_CREAT|O_RDWR);
    if (fd < 0) {
        perror("open()");
        return -1;
    }

    //addr = mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    addr = mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap()");
        close(fd);
        unlink(file_name);
        return -1;
    }

    printf("hugepage[%d]: va 0x%p, pa 0x%lx \n", index, addr, virt_to_phys(addr));

    munmap(addr, MEM_SIZE);
    close(fd);
    unlink(file_name);

    return 0;
}

int main()
{
    int fd;
    int ret;
    char mem_buff[6000];
	char *dyn_buff;
	char *huge_buff;
    int io_data;
    long phy_addr;

	mem_buff[5900] = 0x5a;	
    printf("local: va 0x%p, pa 0x%lx \n", mem_buff, virt_to_phys(mem_buff));

	dyn_buff = (char *)malloc(MEM_SIZE);
	if (dyn_buff) {
		dyn_buff[MEM_SIZE-2] = 0x5a;
        printf("dynamic: va 0x%p, pa 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff));
        free(dyn_buff);
	}

	huge_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), 
	                        (MAP_PRIVATE |MAP_POPULATE | MAP_ANONYMOUS | MAP_HUGETLB), -1, 0);
	if (huge_buff != MAP_FAILED) {
		huge_buff[MEM_SIZE-1] = 0x5a;
        printf("hugepage: va 0x%p, pa 0x%lx \n", huge_buff, virt_to_phys(huge_buff));
        munmap(huge_buff, MEM_SIZE);
	}

    for (int i = 0; i < 3; i++) {
        alloc_huge_page(i);
    }

    fd = open("/dev/misc_mem", O_RDWR);
    if( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

    getchar();
    printf("mmap dma_mem \n");
    io_data = 0; ioctl(fd, 0x100, &io_data);

	dyn_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
	if (dyn_buff != MAP_FAILED) {
        ioctl(fd, 0x200, &phy_addr);
        dyn_buff[0] = 0x11;
		dyn_buff[MEM_SIZE-1] = 0x11;
        printf("map_buff: va 0x%p, pa 0x%lx 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff), phy_addr);
        ioctl(fd, 0x300, &phy_addr);
        munmap(dyn_buff, MEM_SIZE);
	}

    getchar();
    printf("mmap kmalloc \n");
    io_data = 1; ioctl(fd, 0x100, &io_data);

	dyn_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
	if (dyn_buff != MAP_FAILED) {
        ioctl(fd, 0x200, &phy_addr);
        dyn_buff[0] = 0x22;
		dyn_buff[MEM_SIZE-1] = 0x22;
        printf("map_buff: va 0x%p, pa 0x%lx 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff), phy_addr);
        ioctl(fd, 0x300, &phy_addr);
        munmap(dyn_buff, MEM_SIZE);
	}

    getchar();
    printf("mmap glb_mem \n");
    io_data = 2; ioctl(fd, 0x100, &io_data);

	dyn_buff = (char *)mmap(0, MEM_SIZE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
	if (dyn_buff != MAP_FAILED) {
        ioctl(fd, 0x200, &phy_addr);
        //read only ??
        //dyn_buff[0] = 0x33;
		//dyn_buff[MEM_SIZE-1] = 0x33;
        printf("map_buff: va 0x%p, pa 0x%lx 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff), phy_addr);
        ioctl(fd, 0x300, &phy_addr);
        munmap(dyn_buff, MEM_SIZE);
	}

    getchar();
    close(fd);

    return 0;
}



