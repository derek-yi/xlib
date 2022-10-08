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

#define TEXT_SZ 256  
  
struct shared_use_st  
{  
    int  written;
    char text[TEXT_SZ];
};  

int task1_proc()
{
    int running = 1;
    void *shm = NULL;
    struct shared_use_st *shared;
    int shmid;

    //key_t key = ftok("/tmp/myshm1", 0);
    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
    if (shmid == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    shm = shmat(shmid, 0, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Memory attached at 0x%lx \n", (long)shm);
    
    shared = (struct shared_use_st*)shm;
    shared->written = 0;
    while (running)
    {
        if (shared->written == 0)  {
            sleep(1); 
        } else {
            printf("read data: %s", shared->text);
            sleep(1);
            shared->written = 0;
            
            if(strncmp(shared->text, "exit", 3) == 0)
                running = 0;
        }
    }
    
    if (shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    
    if (shmctl(shmid, 0, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

int task2_proc()
{
	int running = 1;
	void *shm = NULL;
	struct shared_use_st *shared = NULL;
	char buffer[TEXT_SZ + 1];
	int shmid;
    
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666|IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
    
	shm = shmat(shmid, (void*)0, 0);
	if (shm == (void*)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory attached at 0x%lx\n", (long)shm);
    
	shared = (struct shared_use_st*)shm;
	while (running)
	{
		while (shared->written == 1) {
			sleep(1);
			printf("Waiting...\n");
		}
        
		printf("Enter some text: ");
		fgets(buffer, TEXT_SZ, stdin);
		strncpy(shared->text, buffer, TEXT_SZ);
        
		shared->written = 1;
		if(strncmp(buffer, "exit", 4) == 0)
			running = 0;
	}
    
	if (shmdt(shm) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    
	sleep(2);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    int ret;
    
    if (argc < 2) {
        printf("\n Usage: %s <1|2>", argv[0]);
        printf("\n   1 -- create read task");
        printf("\n   2 -- create write task");
        printf("\n");
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = task1_proc();
    if (tu == 2) ret = task2_proc();
    
    return ret;
}

/*

gcc -o shm.out shm.c -lrt
./shm.out 2
./shm.out 1

*/


