#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>


sem_t my_sem1;

int vos_sem_clear(void *sem_id)
{
	int cnt, ret;

    if (sem_id == NULL) return 1;
	while (1) {
        cnt = 0;
        ret = sem_getvalue(sem_id, &cnt);
        if ((ret < 0) || (cnt < 1)) break;
        sem_trywait(sem_id);
    }
 
	return 0;
}

void thread_1(void)  
{  
    int value;

    while(1)  {
        sem_wait(&my_sem1);
        //int sem_getvalue(sem_t *restrict sem, int *restrict sval);
        sem_getvalue(&my_sem1, &value);
        printf("pthread_1: value %d \n", value); 
        vos_sem_clear(&my_sem1);
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
void thread_2(void)  
{  
    int value;

    while(1) {
        sem_getvalue(&my_sem1, &value);
        printf("pthread_2: value %d \n", value); 
        sem_post(&my_sem1);
        sem_post(&my_sem1);
        sem_post(&my_sem1);
        sleep(10);  
    }  
    pthread_exit(0);  
}  
  
int case1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

    //int sem_init(sem_t *sem, int pshared, unsigned int value);
    ret = sem_init(&my_sem1, 1, 0);
    if (ret != 0)  {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    ret = pthread_create(&id_1, NULL, (void *)thread_1, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)thread_2, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  
    sem_destroy(&my_sem1);
    return 0;  
}  

int main(int argc, char **argv)
{
    case1_proc();
    return 0;
}




