
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

__thread int my_var = 6;

#if T_DESC("global", 1)
pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 
#endif

#if T_DESC("TU1", 1)
void thread_1(void)  
{  
    int i;  

    for(i=0; i<10; i++)  {
        printf("thread_1: pid=0x%x tid=0x%x self=0x%x\n", getpid(), gettid(), (int)pthread_self());
        my_var += 2;
        printf("thread_1: my_var=%d addr=0x%x \n", my_var, &my_var);
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
void thread_2(void)  
{  
    int i;  
    
    for(i=0; i<5; i++) {
        printf("thread_2: pid=0x%x tid=0x%x self=0x%x\n", getpid(), gettid(), (int)pthread_self());
        my_var += 3;
        printf("thread_2: my_var=%d addr=0x%x \n", my_var, &my_var);
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
int tu1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int ret;  
    unsigned long stack_size;
    pthread_attr_t attr;
    struct sched_param sched;

    pthread_attr_init(&attr);
    sched.sched_priority = 80;
    //pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); //会导致线程创建失败
    
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
    
    ret = pthread_create(&id_2, &attr, (void *)thread_2, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    printf("main: pid=0x%x tid=0x%x self=0x%x\n", getpid(), gettid(), (int)pthread_self());
    
    /*等待线程结束*/  
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  
    return 0;  
}  
#endif

#if T_DESC("TU2", 1)

#endif

#if T_DESC("global", 1)
void usage()
{
    printf("\n Usage: <cmd> <tu>");
    printf("\n    1 -- base case");
    printf("\n    2 -- todo ");
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
    
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
1, compile command
gcc -o thread.out pthread.c -lpthread

*/
#endif


