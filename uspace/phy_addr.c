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

// translate a virtual address to a physical one via /proc/self/pagemap
intptr_t virt_to_phys(void* virt) 
{
	long pagesize = getpagesize(); //sysconf(_SC_PAGESIZE);
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

#define FILE_SIZE       4096

char glb_buff[FILE_SIZE];

int main(int argc, char **argv)
{
    char local_buff[FILE_SIZE];
	char *dyn_buff = NULL;
	unsigned long phy_addr;

	local_buff[FILE_SIZE/2] = 0x11;
    mem_addr(local_buff, &phy_addr);
	printf("local_buff 0x%lx 0x%lx \n", phy_addr, virt_to_phys(local_buff));

	glb_buff[FILE_SIZE/2] = 0x11;
    mem_addr(glb_buff, &phy_addr);
	printf("glb_buff 0x%lx 0x%lx \n", phy_addr, virt_to_phys(glb_buff));

	dyn_buff = malloc(FILE_SIZE);
	if (dyn_buff) {
		dyn_buff[FILE_SIZE/2] = 0x11;
		mem_addr(dyn_buff, &phy_addr);
		printf("dyn_buff 0x%lx 0x%lx \n", phy_addr, virt_to_phys(dyn_buff));
	}

	//while(1) sleep(1);
    return 0;
}

