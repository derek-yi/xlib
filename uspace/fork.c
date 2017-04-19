


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*
#include <unistd.h>
int execl(const char *pathname, const char *arg, …);
int execlp(const char *filename,conxt char *arg, …);
int execle(const char *pathname,conxt char *arg, …,char *const envp[ ])；
int execv(const char *pathname, char *const argv[ ]);
int execvp(const char *filename, char *const argv[ ]);
int execve(const char *pathname, char *const argv[ ],char *const envp[ ]);

#include <stdlib.h>
int system(const char *cmdstring);

#include <sched.h>
int__clone(int (*fn)(void *arg),void *child_stack,int flags,void *arg);

#include <stdlib.h>
void exit(int status);
int atexit(void (*function)(void));
int on_exit(void(*function)(int, void*), void *arg);
void abort(void);

#include <unistd,h>
void _exit(int status);

#include <assert.h>
void assert(int expression);

*/

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

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

*/
#endif


