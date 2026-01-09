#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>


sem_t *my_shm_sem1;
sem_t *my_shm_sem2;

void thread_21(void)  
{  
    int i;  
    for (i=0; i<10; i++)  {
        sem_wait(my_shm_sem1);
        printf("This is pthread_1.\n");  
        sleep(1);  
        printf("pthread_1 sleep ok. \n\n");
        sem_post(my_shm_sem2);
    }  
    pthread_exit(0);  
}  
  
void thread_20(void)  
{  
    int i;  
    for (i=0; i<10; i++) {
        sem_wait(my_shm_sem2);
        printf("This is pthread_0.\n");  
        sleep(1);  
        printf("pthread_0 sleep ok.\n\n");
        sem_post(my_shm_sem1);
    }  
    pthread_exit(0);  
}  
  
int case2_proc(int param)  
{  
    pthread_t thread_id;  
    int ret;  

    //sem_t *sem_open(const char *name,int oflag,mode_t mode,unsigned int value);
    my_shm_sem1 = sem_open("my_sem1", O_CREAT, 644, 0);
    if (my_shm_sem1 == 0)  {  
        printf("sem_open error!\n");  
        return -1;  
    } 

    my_shm_sem2 = sem_open("my_sem2", O_CREAT, 644, 1);
    if (my_shm_sem2 == 0)  {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    if (param) {
        printf("Create thread_21\n");  
        ret = pthread_create(&thread_id, NULL, (void *)thread_21, NULL);  
    } else {
        printf("Create thread_20\n");  
        ret = pthread_create(&thread_id, NULL, (void *)thread_20, NULL);  
    }
    
    if (ret != 0)  {  
        printf("pthread_create error!\n");  
        return -1;  
    }  
    
    pthread_join(thread_id, NULL);  
    sem_close(my_shm_sem1);
    sem_close(my_shm_sem2);
    if ( param ) {
        sem_unlink("my_sem1");
        sem_unlink("my_sem2");
    }
    
    return 0;  
}  

int main(int argc, char **argv)
{
    int ret = 0;
    
    if (argc < 2) {
        printf("usage: %s <0|1> \n", argv[0]);  
        return 0;
    }

    ret = case2_proc(atoi(argv[1]));
    
    return ret;
}

