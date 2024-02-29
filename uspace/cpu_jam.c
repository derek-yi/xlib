#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/syscall.h>

int run_core = 0;
int run_time = 0;
int sched_mode = 0;
int sched_prio = 50;
int diff_us = 1000;

char glb_data[8192];

int vos_bind_to_cpu(int cpu_id)
{
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}

void thread_1(void)  
{  
    pthread_t id_2;  
    int ret; 
    int loop;
    struct timeval pre_ts;
    struct timeval tv;
    struct sched_param param;
    int policy;

    vos_bind_to_cpu(run_core);
    pthread_getschedparam(pthread_self(), &policy, &param);
    printf("thread_1: self 0x%x, policy %d, priority %d \n", 
           (int)pthread_self(), policy, param.sched_priority);
    sleep(3);

    gettimeofday(&tv, NULL);
    pre_ts = tv;
    loop = tv.tv_sec;
    while (tv.tv_sec < loop + run_time) {
        for (int i = 0; i < 8192; i++) {
            glb_data[i] = ((loop * i) + (loop ^ i))%256;
        }

        gettimeofday(&tv, NULL);
        long diff = (tv.tv_sec*1000000 + tv.tv_usec) - (pre_ts.tv_sec*1000000 + pre_ts.tv_usec);
        if (diff > diff_us) {
            printf("warn: diff %ld \n", diff);
        }
        pre_ts = tv;
        //sched_yield();
    }

    pthread_exit(0);  
}  

int pthread_test1(void)  
{  
    pthread_t id_1;  
    int ret;  
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    param.sched_priority = sched_prio;
    if (sched_mode == 1) pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    else if (sched_mode == 2) pthread_attr_setschedpolicy(&attr, SCHED_RR);
    else pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    
    ret = pthread_create(&id_1, &attr, (void *)thread_1, NULL);  
    if (ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    pthread_join(id_1, NULL);  
    return 0;  
}  

int main(int argc, char **argv)
{
    if (argc < 5) {
        printf("usage: %s <core> <time> <diff_us> <0-other|fifo-1|rr-2> [<prio>] \n", argv[0]);
        return 0;
    }

    run_core = atoi(argv[1]);
    run_time = atoi(argv[2]);
    diff_us = atoi(argv[3]);
    sched_mode = atoi(argv[4]);
    if (argc > 5) sched_prio = atoi(argv[5]);

    pthread_test1();
    return 0;
}



