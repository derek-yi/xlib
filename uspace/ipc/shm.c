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

#define SHM_NAME        "shm_ram"
#define FILE_SIZE       4096

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

int tu1_proc(int rw_flag, char *op_str)
{
	int ret = -1;
	int fd = -1;
	char buf[FILE_SIZE] = {0};
	void* map_addr = NULL;

	if (rw_flag == 2) {
		shm_unlink(SHM_NAME);
        return 0;
	} else if (rw_flag == 0) {
		fd = shm_open(SHM_NAME, O_RDWR, 0644);
	} else {
		fd = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0644);
    }
	if (fd < 0) {
		perror("shm_open failed: ");
		goto _OUT;
	}

	if (rw_flag == 1) {
		ret = ftruncate(fd, FILE_SIZE);
		if (-1 == ret) {
			perror("ftruncate failed: ");
			goto _OUT;
		}
	}

	map_addr = mmap(NULL, FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (NULL == map_addr) {
		perror("mmap failed: ");
		goto _OUT;
	}

	printf("map_addr phy_addr: 0x%lx\n", virt_to_phys(map_addr));
    if (rw_flag == 0) { // read
    	memcpy(buf, map_addr, sizeof(buf));
    	printf("read: %s\n", buf);
    } else {  // write
        memcpy(map_addr, op_str, strlen(op_str));
    }

	ret = munmap(map_addr, FILE_SIZE);
	if (-1 == ret) {
		perror("munmap failed: ");
		goto _OUT;
	}
        
_OUT:	
	return ret;
}

int main(int argc, char **argv)
{
    int ret;
	int rw_flag = 1; //write
    
    if(argc < 2) {
        printf("Usage: %s write <str> \r\n", argv[0]);
        printf("       %s read \r\n", argv[0]);
        printf("       %s clean \r\n", argv[0]);
        return 0;
    }

	if (!memcmp(argv[1], "read", 4)) rw_flag = 0;
	else if (!memcmp(argv[1], "clean", 5)) rw_flag = 2;
    ret = tu1_proc(rw_flag,  argv[2]);

    return ret;
}

/*
gcc -o shm.out shm.c -lrt
./shm.out write 337829732
./shm.out read
./shm.out clean
*/

