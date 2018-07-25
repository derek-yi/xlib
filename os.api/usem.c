

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

void thread_1(void)  
{  
    int i;  
    for(i=0; i<10; i++)  {
        sem_wait(&my_sem1);
        printf("This is pthread_1.\n");  
        sleep(1);  
        printf("pthread_1 sleep ok.\n");
        sem_post(&my_sem2);
    }  
    pthread_exit(0);  
}  
  
void thread_2(void)  
{  
    int i;  
    for(i=0; i<10; i++) {
        sem_wait(&my_sem2);
        printf("This is pthread_2.\n");  
        sleep(2);  
        printf("pthread_2 sleep ok.\n");
        sem_post(&my_sem1);
    }  
    pthread_exit(0);  
}  
  
int tu1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

    //int sem_init(sem_t *sem, int pshared, unsigned int value);
    ret = sem_init(&my_sem1, 1, 0);
    if(ret != 0)  
    {  
        printf("sem_init error!\n");  
        return -1;  
    } 

    ret = sem_init(&my_sem2, 1, 1); //value为1，避免死锁
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
    
    /*等待线程结束*/  
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
        printf("pthread_1 sleep ok.\n");
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
        printf("pthread_2 sleep ok.\n");
        sem_post(my_shm_sem1);
    }  
    pthread_exit(0);  
}  
  
int tu2_proc(int argc, char **argv)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  
    int param;

    if (argc < 2) return 1;
    param = atoi(argv[1]);

    //sem_t *sem_open(const char *name,int oflag,mode_t mode,unsigned int value);
    my_shm_sem1 = sem_open("my_sem1", O_CREAT, 644, 0);
    if(my_shm_sem1 == 0)  
    {  
        printf("sem_open error!\n");  
        return -1;  
    } 

    my_shm_sem2 = sem_open("my_sem2", O_CREAT, 644, 1); //value为1，避免死锁
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
    
    /*等待线程结束*/  
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
void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- sem between thread");
    printf("\n   2 -- sem between process, need su mode");
    printf("\n     => P1: 0 - create pid 0; 1 - create pid 1");
    printf("\n");
}

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        usage();
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = tu1_proc();
    if (tu == 2) ret = tu2_proc(argc - 1, &argv[1]);
    
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
1, how to compile 
gcc -o usem.out usem.c -lpthread
gcc -g -o usem.out usem.c -lpthread

*/
#endif

