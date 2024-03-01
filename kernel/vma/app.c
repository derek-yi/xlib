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


#define DMA_BUFF_SZ 1024

static char global_data1[4096];
static char global_data2[409600];
char bss_data[4096];

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
		//printf("failed to translate virtual address %p to physical address \n", virt);
	}
    
	// bits 0-54 are the page number
	return (phy & 0x7fffffffffffffULL) * pagesize + ((intptr_t) virt) % pagesize;
}

int main()
{
    int fd, ret;
    int stack_data = 1;
    int mem_buff[4096];
    char *map_mem;
    void *phy_addr;
    static int static_data = 1;
    char* malloc_data    = malloc(16);
    char* malloc_data1   = malloc(512);
    char* malloc_data2   = malloc(512*1024);

    fd = open("/dev/vma_demo", O_RDWR);
    if ( fd < 0 ) {
        printf("open failed \n");
        return 0;
    }

	map_mem = (char *)mmap(NULL, DMA_BUFF_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_mem != MAP_FAILED) {
		//memset(map_mem, 0x10, DMA_BUFF_SZ);
        printf("map_map 0x%x 0x%x 0x%x \n", map_mem[0], map_mem[1], map_mem[2]);
        phy_addr = virt_to_phys(map_mem); //map failed
		printf("map_map 0x%p, phy_addr 0x%lx \n", map_mem, phy_addr);
	}

	phy_addr = virt_to_phys(mem_buff);
	printf("local va 0x%p, pa 0x%p \n", mem_buff, phy_addr);

    //code segment
    printf("pid %d \n", getpid());
    printf("code segment:\n");
    printf("\t code_data=0x%lx\n", (long)main);
 
    //data segment
    printf("data segment:\n");
    phy_addr = virt_to_phys(global_data1); //map failed
    printf("\t global_data1=0x%lx 0x%lx\n", (long)global_data1, phy_addr);
    printf("\t global_data2=0x%lx\n", (long)global_data2);
    printf("\t static_data=0x%lx\n",  (long)&static_data);
 
    //bss segment
    printf("bss segment:\n");
    printf("\t bss_data=0x%lx\n", (long)bss_data);

    // stack segment
    printf("stack segment:\n");
    printf("\t stack_data=0x%lx\n",     (long)&stack_data);
 
    // heap segment
    printf("heap segment:\n");
    printf("\t malloc_data=0x%lx\n",  (long)malloc_data);
    printf("\t malloc_data1=0x%lx 0x%lx\n", (long)malloc_data1, virt_to_phys(malloc_data1));
    printf("\t malloc_data2=0x%lx 0x%lx\n", (long)malloc_data2, virt_to_phys(malloc_data2));

	while(1) sleep(1);
    if (map_mem != MAP_FAILED) munmap(map_mem, DMA_BUFF_SZ);
    close(fd);
    return 0;
}

/*
root@linaro-alip:/home/unisoc# ./a.out
map_map 0x0 0x1 0x2
map_map 0x0x7ff7ff7000, phy_addr 0x0
local va 0x0x7fffffb798, pa 0x0x43ad798
pid 87782
code segment:
         code_data=0x5555550afc
data segment:
         global_data1=0x5555563098 0x98
         global_data2=0x5555564098
         static_data=0x5555562088
bss segment:
         bss_data=0x5555562098
stack segment:
         stack_data=0x7ffffff79c
heap segment:
         malloc_data=0x55555c92a0
         malloc_data1=0x55555c92c0 0x2db802c0

cat /sys/devices/virtual/misc/vma_demo/cmd_buffer
echo show > /sys/devices/virtual/misc/vma_demo/cmd_buffer

root@linaro-alip:/home/unisoc# echo 87782 > /sys/devices/virtual/misc/vma_demo/cmd_buffer
root@linaro-alip:/home/unisoc# dmesg
[ 4054.249677] current pid 2455
[ 4054.249698] mm_struct addr = 0x000000005657a49d
[ 4054.249703]      1:         a67dc784   5555550000   5555552000     8192 flag 0x875
[ 4054.249708]      2:         ddf495f7   5555561000   5555562000     4096 flag 0x100871
[ 4054.249712]      3:         e1a9d5fe   5555562000   5555563000     4096 flag 0x100873
[ 4054.249716]      4:         e072f598   5555563000   55555ea000   552960 flag 0x100073
[ 4054.249720]      5:         8eb2fbde   7ff7dbd000   7ff7e3e000   528384 flag 0x100073
[ 4054.249724]      6:         f6ec28b0   7ff7e3e000   7ff7f9a000  1425408 flag 0x75
[ 4054.249728]      7:         a299a715   7ff7f9a000   7ff7fa9000    61440 flag 0x70
[ 4054.249731]      8:         93892b81   7ff7fa9000   7ff7fad000    16384 flag 0x100071
[ 4054.249735]      9:         febff3e3   7ff7fad000   7ff7faf000     8192 flag 0x100073
[ 4054.249739]     10:         c01d770d   7ff7faf000   7ff7fb2000    12288 flag 0x100073
[ 4054.249742]     11:         4dd35d49   7ff7fcc000   7ff7fed000   135168 flag 0x875
[ 4054.249746]     12:         8eab9d62   7ff7ff7000   7ff7ff8000     4096 flag 0x40444fb
[ 4054.249750]     13:         e6b833cb   7ff7ff8000   7ff7ffa000     8192 flag 0x100073
[ 4054.249754]     14:         9ec1801a   7ff7ffa000   7ff7ffc000     8192 flag 0x40411
[ 4054.249757]     15:         a718bef5   7ff7ffc000   7ff7ffd000     4096 flag 0x40075
[ 4054.249761]     16:         94c541c1   7ff7ffd000   7ff7ffe000     4096 flag 0x100871
[ 4054.249765]     17:         40b823a5   7ff7ffe000   7ff8000000     8192 flag 0x100873
[ 4054.249768]     18:         3d07b433   7ffffdf000   8000000000   135168 flag 0x100173




*/

