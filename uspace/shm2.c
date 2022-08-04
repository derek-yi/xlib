#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define TEXT_SZ         256  
#define WRITE_STR       "to be or not to be"

struct shared_use_st  
{  
    char text[TEXT_SZ];
};  

int tu1_proc(char *op_code, char *op_str)
{
	int ret = -1;
    int shmid;
	char buf[4096] = {0};
    void *shm = NULL;

    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
    if(shmid == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    shm = shmat(shmid, 0, 0);
    if(shm == (void*)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Memory attached at 0x%lx \n", (long)shm);

    if (!strcmp(op_code, "read")) { // read
    	memcpy(buf, shm, sizeof(buf));
    	printf("read: %s\n", buf);
    } else {  // write
        if (op_str) memcpy(shm, op_str, strlen(op_str));
        else memcpy(shm, WRITE_STR, strlen(WRITE_STR));
    }

    if(shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
        
_OUT:	
    if (!strcmp(op_code, "read")) shmctl(shmid, 0, 0);
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


