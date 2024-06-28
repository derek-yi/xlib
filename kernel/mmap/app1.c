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
#include <stdint.h>

#define MAP_SIZE 	            (4096)
#define MAP_MASK                (MAP_SIZE - 1)

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

/*
pagemap is a new (as of 2.6.25) set of interfaces in the kernel that allow
userspace programs to examine the page tables and related information by
reading files in /proc.

There are four components to pagemap:

 * /proc/pid/pagemap.  This file lets a userspace process find out which
   physical frame each virtual page is mapped to.  It contains one 64-bit
   value for each virtual page, containing the following data (from
   fs/proc/task_mmu.c, above pagemap_read):

    * Bits 0-54  page frame number (PFN) if present
    * Bits 0-4   swap type if swapped
    * Bits 5-54  swap offset if swapped
    * Bit  55    pte is soft-dirty (see Documentation/vm/soft-dirty.txt)
    * Bit  56    page exclusively mapped (since 4.2)
    * Bits 57-60 zero
    * Bit  61    page is file-page or shared-anon (since 3.5)
    * Bit  62    page swapped
    * Bit  63    page present

   Since Linux 4.0 only users with the CAP_SYS_ADMIN capability can get PFNs.
   In 4.0 and 4.1 opens by unprivileged fail with -EPERM.  Starting from
   4.2 the PFN field is zeroed if the user does not have CAP_SYS_ADMIN.
   Reason: information about PFNs helps in exploiting Rowhammer vulnerability.

   If the page is not present but in swap, then the PFN contains an
   encoding of the swap file number and the page's offset into the
   swap. Unmapped pages return a null PFN. This allows determining
   precisely which pages are mapped (or in swap) and comparing mapped
   pages between processes.

   Efficient users of this interface will use /proc/pid/maps to
   determine which areas of memory are actually mapped and llseek to
   skip over unmapped regions.
*/
//https://www.cnblogs.com/pengdonglin137/p/6802108.html
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


//计算虚拟地址对应的地址，传入虚拟地址vaddr，通过paddr传出物理地址
void mem_addr(unsigned long vaddr, unsigned long *paddr)
{
	int pageSize = getpagesize();//调用此函数获取系统设定的页面大小
	unsigned long v_pageIndex = vaddr / pageSize;//计算此虚拟地址相对于0x0的经过的页面数
	unsigned long v_offset = v_pageIndex * sizeof(uint64_t);//计算在/proc/pid/page_map文件中的偏移量
	unsigned long page_offset = vaddr % pageSize;//计算虚拟地址在页面中的偏移量
	uint64_t item = 0;//存储对应项的值

	int fd = open("/proc/self/pagemap", O_RDONLY); //以只读方式打开/proc/pid/page_map
	if (fd == -1)//判断是否打开失败
	{
		printf("open /proc/self/pagemap error\n");
		return;
	}

	if (lseek(fd, v_offset, SEEK_SET) == -1)//将游标移动到相应位置，即对应项的起始地址且判断是否移动失败
	{
		printf("lseek error\n");
		return;	
	}

	if (read(fd, &item, sizeof(uint64_t)) != sizeof(uint64_t))//读取对应项的值，并存入item中，且判断读取数据位数是否正确
	{
		printf("read item error\n");
		return;
	}

	if ((((uint64_t)1 << 63) & item) == 0)//判断present是否为0
	{
		printf("page present is 0\n");
		return ;
	}

	uint64_t phy_pageIndex = (((uint64_t)1 << 55) - 1) & item;//计算物理页号，即取item的bit0-54

	*paddr = (phy_pageIndex * pageSize) + page_offset;//再加上页内偏移量就得到了物理地址
}

int main(int argc, char **argv)
{
    int fd, ret;  
	char *dyn_buff;
    char *mapBuf;  
    char *shm_buff;  
    unsigned long phy_addr = 0;

	dyn_buff = (char *)malloc(MAP_SIZE);
	if (dyn_buff) {
		dyn_buff[MAP_SIZE-2] = 0x5a;
        printf("dynamic: va 0x%p, pa 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff));
        if (dyn_buff) free(dyn_buff);
	}

    fd = open("/dev/misc_xx", O_RDWR);
    if (fd < 0)  {  
        printf("open /dev/misc_xx failed \n");  
        return -1;  
    }  
    
    mapBuf = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE|MAP_LOCKED, fd, 0);
    if (mapBuf == NULL) {
        printf("mmap failed \n");
        close(fd);
        return 0;  
    }

    for (int i = 0; i < 5; i++) {
        printf("mapBuf[%d] %d \n", i, mapBuf[i]);
        mapBuf[i] += 1;
    }

    mem_addr(mapBuf, &phy_addr);
    printf("mapBuf: va 0x%p, pa 0x%lx 0x%lx \n", mapBuf, virt_to_phys(mapBuf), phy_addr);
    munmap(mapBuf, MAP_SIZE);
    close(fd);

    fd = shm_open("shm_demo", O_RDWR | O_CREAT, 0666);
    if (fd > 0) {
        ret  = ftruncate(fd, MAP_SIZE);
        if (-1 == ret) {
            perror("ftruncate failed: ");
            return;
        }
        
        shm_buff = (char*)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (NULL == shm_buff) {
            perror("mmap failed: ");
            return;
        }
        for (int i = 0; i < 5; i++) {
            printf("shm_buff[%d] %d \n", i, shm_buff[i]);
            shm_buff[i] += 1;
        }

        mem_addr(shm_buff, &phy_addr);
        printf("shm_buff: va 0x%p, pa 0x%lx 0x%lx \n", shm_buff, virt_to_phys(shm_buff), phy_addr);
        munmap(shm_buff, MAP_SIZE);
    }

    return 0;
}


