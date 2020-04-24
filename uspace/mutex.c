


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*
#include <pthread.h>

pthread_mutex_t fastmutex=PTHREAD_MUTEX_INITEALIZER;
pthread_mutex_t recmutex=PTHREAD_RECURSIVE_MUTEX_INITEALIZER_NP;
pthread_mutex_t errchkmutex=PTHREAD_ERRORCHECK_MUTEX_INITEALIZER_NP;

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

*/

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void thread_1(void)  
{  
    int i; 
    
    for(i=0; i<10; i++)  {
        pthread_mutex_lock(&mutex1);
        printf("This is thread_1...");  
        sleep(2);  
        printf("111.\n");
        pthread_mutex_unlock(&mutex1);
        //sleep(1);  
    }  
    pthread_exit(0);  
}  
  
void thread_2(void)  
{  
    int i;  
    for(i=0; i<10; i++) {
        pthread_mutex_lock(&mutex1);
        printf("This is thread_2...");  
        sleep(2);  
        printf("222.\n");
        pthread_mutex_unlock(&mutex1);
        //sleep(1);  
    }  
    pthread_exit(0);  
}  

int tu1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

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

    return 0;  
}  

#endif

#if T_DESC("TU2", 1)

pthread_mutex_t mutex3;

void thread_3(void)  
{  
    int i;  
    for(i=0; i<10; i++)  {
        pthread_mutex_lock(&mutex3);
        printf("This is thread_3...");  
        sleep(2);  
        printf("sleep done.\n");
        
        pthread_mutex_lock(&mutex3);
        printf("This is thread_3...");  
        sleep(2);  
        printf("sleep done.\n");
        
        pthread_mutex_unlock(&mutex3);
        sleep(1);  
        printf("pthread_mutex_unlock 1.\n");
        
        pthread_mutex_unlock(&mutex3);
        sleep(1);  
        printf("pthread_mutex_unlock 2.\n");
    }  
    pthread_exit(0);  
}  
  
void thread_4(void)  
{  
    int i;  
    for(i=0; i<10; i++) {
        pthread_mutex_lock(&mutex3);
        printf("This is thread_4...");  
        sleep(2);  
        printf("sleep done.\n");
        pthread_mutex_unlock(&mutex3);
        sleep(1);  
    }  
    pthread_exit(0);  
}  

int tu2_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  
    pthread_mutexattr_t mutexattr;

    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex3, &mutexattr);
    
    ret = pthread_create(&id_1, NULL, (void *)thread_3, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)thread_4, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    /*等待线程结束*/  
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  

    return 0;  
}  

#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu>");
    printf("\n   1 -- NORMAL_MUTEX");
    printf("\n   2 -- RECURSIVE_MUTEX");
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
    if (tu == 2) ret = tu2_proc();
    
    return ret;
}
#endif

