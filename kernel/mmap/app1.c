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

#define MEM_SIZE 	(4096)

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )


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

int main()
{
    int fd, ret;  
	char *dyn_buff;
    char *mapBuf;  
    char *shm_buff;  
    unsigned long phy_addr;

	dyn_buff = (char *)malloc(MEM_SIZE);
	if (dyn_buff) {
		dyn_buff[MEM_SIZE-2] = 0x5a;
        printf("dynamic: va 0x%p, pa 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff));
        if (dyn_buff) free(dyn_buff);
	}

    fd = open("/dev/misc_xx", O_RDWR);
    if (fd < 0)  {  
        printf("open failed, fd %d \n",fd);  
        return -1;  
    }  
    
    mapBuf = mmap(NULL, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
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
    munmap(mapBuf, MEM_SIZE);
    close(fd);

    fd = shm_open("shm_demo", O_RDWR | O_CREAT, 0666);
    if (fd > 0) {
        ret  = ftruncate(fd, MEM_SIZE);
        if (-1 == ret) {
            perror("ftruncate failed: ");
            return;
        }
        
        shm_buff = (char*)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (NULL == shm_buff) {
            perror("mmap failed: ");
            return;
        }
        for (int i = 0; i < 5; i++) {
            shm_buff[i] += 1;
        }

        mem_addr(shm_buff, &phy_addr);
        printf("shm_buff: va 0x%p, pa 0x%lx 0x%lx \n", shm_buff, virt_to_phys(shm_buff), phy_addr);
        munmap(shm_buff, MEM_SIZE);
    }

    return 0;
}


