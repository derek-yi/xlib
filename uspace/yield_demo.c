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
#include <sys/ioctl.h>

char glb_data[8192];

uint64_t rr_cnt = 0;

uint64_t norm_cnt = 0;

int loop_enable = 1;

void vos_msleep(uint32_t milliseconds) 
{
    struct timespec ts = {
        milliseconds / 1000,
        (milliseconds % 1000) * 1000000
    };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

int vos_bind_to_cpu(int cpu_id)
{
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}

void thread_1(void)  
{  
    vos_bind_to_cpu(4);
    sleep(2);
    printf("thread_1: pid=%d tid=%d self=%d\n", getpid(), gettid(), (int)pthread_self());
    printf("thread_1: %d \r\n", sched_getscheduler(0));
    
    while (loop_enable) {
        norm_cnt++;
    }
    
    pthread_exit(0);  
}  
  

void thread_2(void)  
{  
    struct sched_param param;
    int loop = 0x5a;

    param.sched_priority = 30;
    sched_setscheduler(0, SCHED_RR, &param);
    vos_bind_to_cpu(4);
    sleep(2);
    printf("thread_2: pid=%d tid=%d self=%d\n", getpid(), gettid(), (int)pthread_self());
    printf("thread_2: %d \r\n", sched_getscheduler(0));
    
    while (loop_enable) {
        for (int i = 0; i < 8192; i++) {
            glb_data[i] += ((loop * i) + (loop ^ i))%256;
        }

        rr_cnt++;
        sched_yield();
    }  
    pthread_exit(0);  
}  

void thread_3(void)  
{  
    for (int i = 0; i < 10; i++)  {
        printf("thread_3: rr_cnt %lu norm_cnt %lu \n", rr_cnt, norm_cnt);
        sleep(5);
    }  
    loop_enable = 0;
}  

int pthread_test1(void)  
{  
    pthread_t id_1, id_2, id_3;  
    int ret;  
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    ret = pthread_create(&id_1, &attr, (void *)thread_1, NULL);  
    if (ret != 0) {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    ret = pthread_create(&id_2, &attr, (void *)thread_2, NULL);  
    if (ret != 0) {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    ret = pthread_create(&id_3, &attr, (void *)thread_3, NULL);  
    if (ret != 0) {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    printf("main: pid=0x%x tid=0x%x self=0x%x\n", getpid(), gettid(), (int)pthread_self());
    
    pthread_join(id_3, NULL);  
    
    return 0;  
}  

//sysctl -w kernel.sched_rt_runtime_us=-1
//sysctl -w kernel.sched_rt_runtime_us=950000
int main(int argc, char **argv)
{
    pthread_test1();
    return 0;
}

