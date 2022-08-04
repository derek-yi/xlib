#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define SHM_NAME        "shm_ram"
#define FILE_SIZE       4096
#define WRITE_STR       "to be or not to be"

int tu1_proc(char *op_code, char *op_str)
{
	int ret = -1;
	int fd = -1;
	char buf[4096] = {0};
	void* map_addr = NULL;

	fd = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0644);
	if(fd < 0) {
		perror("shm  failed: ");
		goto _OUT;
	}
	
	ret = ftruncate(fd, FILE_SIZE);
	if(-1 == ret) {
		perror("ftruncate failed: ");
		goto _OUT;
	}
	
	map_addr = mmap(NULL, FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(NULL == map_addr) {
		perror("mmap failed: ");
		goto _OUT;
	}

    if (!strcmp(op_code, "read")) { // read
    	memcpy(buf, map_addr, sizeof(buf));
    	printf("read: %s\n", buf);
    } else {  // write
        if (op_str) memcpy(map_addr, op_str, strlen(op_str));
        else memcpy(map_addr, WRITE_STR, strlen(WRITE_STR));
    }

	ret = munmap(map_addr, FILE_SIZE);
	if(-1 == ret) {
		perror("munmap faile: ");
		goto _OUT;
	}
        
_OUT:	
    if (!strcmp(op_code, "read")) shm_unlink(SHM_NAME);
	return ret;
}

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        printf("\n Usage: %s write <str> \r\n", argv[0]);
        printf("\n Usage: %s read \r\n", argv[0]);
        return 0;
    }

    ret = tu1_proc(argv[1],  argv[2]);
    
    return ret;
}

/*
gcc -o shm.out shm.c -lrt
./shm.out write aaa
./shm.out read
*/

