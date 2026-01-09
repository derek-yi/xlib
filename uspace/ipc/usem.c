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
sem_t my_sem2;

int global_var = 0;

int func_a(char *thread, int value)
{
    sem_wait(&my_sem1);
    global_var += value;
    printf("func_a for %s begin: var=%d \n", thread, global_var); 
    sleep(1);  
    printf("func_a for %s end: var=%d \n", thread, global_var); 
    sem_post(&my_sem1);
}

int func_b(char *thread)
{
    sem_wait(&my_sem2);
    printf("func_b for %s begin \n", thread); 
    sleep(1);  
    printf("func_b for %s end \n", thread); 
    sem_post(&my_sem2);
}


void thread_1(void)  
{  
    while (1)  {
        printf("This is pthread_1.\n");  
        func_a("thread_1", 1000);
        //func_b("thread_1 ");
        printf("pthread_1 end. \n\n");
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
void thread_2(void)  
{  
    while (1) {
        printf("This is pthread_2.\n");  
        //func_b("thread_2");
        func_a("thread_2", 1);
        printf("pthread_2 end. \n\n");
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
int case1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

    //int sem_init(sem_t *sem, int pshared, unsigned int value);
    ret = sem_init(&my_sem1, 1, 1);
    if (ret != 0)  {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    ret = sem_init(&my_sem2, 1, 1);
    if (ret != 0) {  
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
    sem_destroy(&my_sem2);
    return 0;  
}  

int main(int argc, char **argv)
{
    case1_proc();
    return 0;
}




