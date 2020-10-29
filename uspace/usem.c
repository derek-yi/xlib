

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

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
    while(1)  {
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
    while(1) {
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
    if(ret != 0)  
    {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    ret = sem_init(&my_sem2, 1, 1);
    if(ret != 0)  
    {  
        printf("sem_init error!\n");   
        return -1;  
    } 
    
    ret = pthread_create(&id_1, NULL, (void *)thread_1, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)thread_2, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  

    sem_destroy(&my_sem1);
    sem_destroy(&my_sem2);
    
    return 0;  
}  
#endif

#if T_DESC("TU2", 1)

sem_t *my_shm_sem1;
sem_t *my_shm_sem2;

void thread_21(void)  
{  
    int i;  
    for(i=0; i<10; i++)  {
        sem_wait(my_shm_sem1);
        printf("This is pthread_1.\n");  
        sleep(1);  
        printf("pthread_1 sleep ok. \n\n");
        sem_post(my_shm_sem2);
    }  
    pthread_exit(0);  
}  
  
void thread_22(void)  
{  
    int i;  
    for(i=0; i<10; i++) {
        sem_wait(my_shm_sem2);
        printf("This is pthread_2.\n");  
        sleep(1);  
        printf("pthread_2 sleep ok.\n\n");
        sem_post(my_shm_sem1);
    }  
    pthread_exit(0);  
}  
  
int case2_proc(int argc, char **argv)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  
    int param;

    if (argc < 3) return 1;
    param = atoi(argv[2]);

    //sem_t *sem_open(const char *name,int oflag,mode_t mode,unsigned int value);
    my_shm_sem1 = sem_open("my_sem1", O_CREAT, 644, 0);
    if(my_shm_sem1 == 0)  
    {  
        printf("sem_open error!\n");  
        return -1;  
    } 

    my_shm_sem2 = sem_open("my_sem2", O_CREAT, 644, 1);
    if(my_shm_sem2 == 0)  
    {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    if (param) {
        ret = pthread_create(&id_1, NULL, (void *)thread_21, NULL);  
    } else {
        ret = pthread_create(&id_2, NULL, (void *)thread_22, NULL);  
    }
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    if (param) {
        pthread_join(id_1, NULL);  
    } else {
        pthread_join(id_2, NULL);  
    }

    sem_close(my_shm_sem1);
    sem_close(my_shm_sem2);
    
    if (param ) {
        sem_unlink("my_sem1");
        sem_unlink("my_sem2");
    }
    
    return 0;  
}  

#endif

#if T_DESC("global", 1)

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        printf("Usage: \n");
        printf(" %s 1      -- sem between thread \n", argv[0]);
        printf(" %s 2 <p1> -- sem between process, p1=<0,1> as pid \n", argv[0]);
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = case1_proc();
    if (tu == 2) ret = case2_proc(argc, argv);
    
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
gcc -o usem.out usem.c -lpthread
gcc -g -o usem.out usem.c -lpthread

sudo ./usem.out 1
sudo ./usem.out 2 0 &
sudo ./usem.out 2 1


*/
#endif

