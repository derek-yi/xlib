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

#define MEM_SIZE 	(16*1024)

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

#if 0
//https://www.cnblogs.com/pengdonglin137/p/6802108.html
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

unsigned long virt_to_phys(unsigned long virt_addr)
{
    FILE *f;
    uint64_t read_val, file_offset, page_size, phy_addr;
    int status, c, i;
    
    //printf("Big endian? %d\n", is_bigendian());
    f = fopen("/proc/self/pagemap", "rb");
    if(!f){
        printf("Error! Cannot open pagemap\n");
        return 0;
    }

    page_size = getpagesize();
    //Shifting by virt-addr-offset number of bytes
    //and multiplying by the size of an address (the size of an entry in pagemap file)
    file_offset = virt_addr / page_size * PAGEMAP_ENTRY;
    //printf("Vaddr: 0x%lx, Page_size: %lld, Entry_size: %d\n", virt_addr, page_size, PAGEMAP_ENTRY);
    //printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);
    status = fseek(f, file_offset, SEEK_SET);
    if(status){
        printf("Failed to do fseek \n");
        fclose(f);
        return 0;
    }

    read_val = 0;
    unsigned char c_buf[PAGEMAP_ENTRY];
    for(i=0; i < PAGEMAP_ENTRY; i++){
        c = getc(f);
        if(c==EOF){
            printf("Reach end of the file\n");
            return 0;
        }
        if(is_bigendian())
            c_buf[i] = c;
        else
            c_buf[PAGEMAP_ENTRY - i - 1] = c;
        //printf("[%d]0x%x ", i, c);
    }
    for(i=0; i < PAGEMAP_ENTRY; i++){
        //printf("%d ",c_buf[i]);
        read_val = (read_val << 8) + c_buf[i];
    }

    //printf("Result: 0x%llx\n", (unsigned long long) read_val);
    if(GET_BIT(read_val, 63)) {
        uint64_t pfn = GET_PFN(read_val);
        //printf("PFN: 0x%llx (0x%llx)\n", pfn, pfn * page_size + virt_addr % page_size);
        phy_addr = pfn * page_size + virt_addr % page_size;
    } else {
        printf("Page not present\n");
        phy_addr = 0;
    }

    //if(GET_BIT(read_val, 62)) printf("Page swapped\n");
    fclose(f);
    return phy_addr;
}

#else
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

#endif

int main()
{
    int ret;
    char mem_buff[6000];
	char *dyn_buff;
 
	mem_buff[5900] = 0x5a;	
    printf("local: va 0x%p, pa 0x%lx \n", mem_buff, virt_to_phys(mem_buff));

	dyn_buff = (char *)malloc(MEM_SIZE);
	if (dyn_buff) {
		dyn_buff[MEM_SIZE-2] = 0x5a;
        printf("dynamic: va 0x%p, pa 0x%lx \n", dyn_buff, virt_to_phys(dyn_buff));
	}

    getchar();
    if (dyn_buff) free(dyn_buff);
    return 0;
}


