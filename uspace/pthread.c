
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


__thread int my_var = 6;

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

void thread_2(void)  
{  
    struct sched_param param;

    param.sched_priority = 80;
    sched_setscheduler(0, SCHED_RR, &param);
    
    while(1)  {
        printf("thread_2: ppid=%d pid=%d tid=%d self=%d\n", getppid(), getpid(), gettid(), (int)pthread_self());
        printf("thread_2: %d %d \r\n", sched_getscheduler(0), sched_getscheduler(gettid()));
        my_var += 1000;
        printf("thread_2: my_var=%d addr=0x%x \n", my_var, &my_var);
        sleep(10);
    }  
    pthread_exit(0);  
}  

void thread_1(void)  
{  
    pthread_t id_2;  
    int ret;  

    printf("thread_1: %d %d \r\n", sched_getscheduler(0), sched_getscheduler(gettid()));
    
    ret = pthread_create(&id_2, NULL, (void *)thread_2, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }      
    
    while(1)  {
        printf("thread_1: ppid=%d pid=%d tid=%d self=%d\n", getppid(), getpid(), gettid(), (int)pthread_self());
        my_var += 1;
        printf("thread_1: my_var=%d addr=0x%x \n", my_var, &my_var);
        sleep(10);
    }  
    pthread_exit(0);  
}  
  
#include <sched.h>


int pthread_test1(void)  
{  
    pthread_t id_1,id_2;  
    int ret;  
    unsigned long stack_size;
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    param.sched_priority = 80;
    //pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    
    pthread_attr_getstacksize(&attr, &stack_size);
    printf("default stack size is %ld(k)\n", stack_size/1024);
    printf("SCHED_FIFO: Max %u, Min %u\n", sched_get_priority_max(SCHED_FIFO), sched_get_priority_min(SCHED_FIFO));
    printf("SCHED_RR: Max %u, Min %u\n", sched_get_priority_max(SCHED_RR), sched_get_priority_min(SCHED_RR));
    printf("SCHED_OTHER: Max %u, Min %u\n", sched_get_priority_max(SCHED_OTHER), sched_get_priority_min(SCHED_OTHER));

    ret = pthread_create(&id_1, &attr, (void *)thread_1, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    printf("main: pid=0x%x tid=0x%x self=0x%x\n", getpid(), gettid(), (int)pthread_self());
    
    pthread_join(id_1, NULL);  
    //pthread_join(id_2, NULL);  
    
    return 0;  
}  


int main(int argc, char **argv)
{
    pthread_test1();
    return 0;
}



