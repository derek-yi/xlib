#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define TEXT_SZ 256  
  
struct shared_st  
{  
    int  written;
    char text[TEXT_SZ];
};  

char *shm_mem = NULL;

void task1_proc(void)
{
    int running = 1;
    struct shared_st *shared;

    shared = (struct shared_st*)shm_mem;
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
        
    exit(EXIT_SUCCESS);
}

void task2_proc(void)
{
	int running = 1;
	struct shared_st *shared = NULL;
	char buffer[TEXT_SZ + 1];

	shared = (struct shared_st*)shm_mem;
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
    
	sleep(2);
	exit(EXIT_SUCCESS);
}

int main (int argc, char **argv) 
{
    pthread_t unused_id;
    
    shm_mem = (char *) mmap(NULL, sizeof(struct shared_st), 
                            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_mem == NULL) {
        perror("mmap");
        exit (0);
    } 

    pthread_create(&unused_id, NULL, (void *)task1_proc, NULL);  
    pthread_create(&unused_id, NULL, (void *)task2_proc, NULL);  

    pthread_join(unused_id, NULL);  
    munmap(shm_mem, sizeof(struct shared_st));
    return 0;
}

