#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/ioctl.h>

int bind_cpu = 0;

int vos_bind_to_cpu(int cpu_id)
{
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
}

#if 0
int delay_test(char *task_name)
{
    struct timeval start_tv, end_tv;   
    long diff_us;
    
    gettimeofday(&start_tv, NULL);
    usleep(1);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: usleep(1) %d us \n", task_name, diff_us);

    gettimeofday(&start_tv, NULL);
    usleep(5);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: usleep(5) %d us \n", task_name, diff_us);

    gettimeofday(&start_tv, NULL);
    usleep(10);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: usleep(10) %d us \n", task_name, diff_us);

    gettimeofday(&start_tv, NULL);
    usleep(1000);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: usleep(1000) %d us \n", task_name, diff_us);

    gettimeofday(&start_tv, NULL);
    sched_yield();
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: sched_yield() %d us \n", task_name, diff_us);

    return 0;
}

#else

static sem_t demo_sem; 
static pthread_cond_t cond;
static pthread_mutex_t mutex;

int delay_test(char *task_name)
{
    struct timeval start_tv, end_tv;   
    long diff_us;
	struct timespec ts;
    int ret;
    
    gettimeofday(&start_tv, NULL);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 1000;  //1us
	if (ts.tv_nsec >= 1000*1000*1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000*1000*1000;
    } 
	ret = sem_timedwait(&demo_sem, &ts);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: sem_timedwait(1) %d us, ret %d \n", task_name, diff_us, ret);

    gettimeofday(&start_tv, NULL);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 10000;  //10us
	if (ts.tv_nsec >= 1000*1000*1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000*1000*1000;
    } 
	ret = sem_timedwait(&demo_sem, &ts);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: sem_timedwait(10) %d us, ret %d \n", task_name, diff_us, ret);

    gettimeofday(&start_tv, NULL);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 50000;  //50us
	if (ts.tv_nsec >= 1000*1000*1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000*1000*1000;
    } 
	ret = sem_timedwait(&demo_sem, &ts);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: sem_timedwait(50) %d us, ret %d \n", task_name, diff_us, ret);

    gettimeofday(&start_tv, NULL);
    pthread_mutex_lock(&mutex);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 10000;  //10us
	if (ts.tv_nsec >= 1000*1000*1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000*1000*1000;
    } 
	ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    pthread_mutex_unlock(&mutex);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: pthread_cond_timedwait(10) %d us, ret %d \n", task_name, diff_us, ret);

    gettimeofday(&start_tv, NULL);
    pthread_mutex_lock(&mutex);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec += 50000;  //50us
	if (ts.tv_nsec >= 1000*1000*1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000*1000*1000;
    } 
	ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    pthread_mutex_unlock(&mutex);
    gettimeofday(&end_tv, NULL);
    diff_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + (end_tv.tv_usec - start_tv.tv_usec);
    printf("%s: pthread_cond_timedwait(50) %d us, ret %d \n", task_name, diff_us, ret);

    return 0;
}

#endif

void thread_2(void)  
{  
    struct sched_param param;

    vos_bind_to_cpu(bind_cpu);
    param.sched_priority = 50;
    sched_setscheduler(0, SCHED_FIFO, &param);
    sleep(1);
    
    printf("thread_2: pid=%d tid=%d self=%d\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_2: scheduler %d \r\n", sched_getscheduler(0));
    sleep(1);
    
    for (int i = 0; i < 5; i++) {
        delay_test("SCHED_FIFO");
    }
}

void thread_1(void)  
{  
    struct sched_param param;

    vos_bind_to_cpu(bind_cpu);
    param.sched_priority = 50;
    sched_setscheduler(0, SCHED_OTHER, &param);
    sleep(1);
    
    printf("thread_1: pid=%d tid=%d self=%d\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_1: scheduler %d \r\n", sched_getscheduler(0));
    sleep(1);

    for (int i = 0; i < 5; i++) {
        delay_test("SCHED_OTHER");
    }
}

int main(int argc, char **argv)
{
    pthread_t my_tid;
    int ret, select;
    
    if (argc < 3) {
        printf("%s <core> <0-other|1-fifo> \n", argv[0]);
        return 0;
    }

    bind_cpu = atoi(argv[1]);
    select = atoi(argv[2]);
    printf("main: pid=%d tid=%d self=%d\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("main: scheduler %d \r\n", sched_getscheduler(0));

    sem_init(&demo_sem, 0, 0); 
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    if (select == 0) {
        ret = pthread_create(&my_tid, NULL, (void *)thread_1, NULL);  
        if (ret != 0)  {  
            printf("Create pthread error!\n");  
            return -1;  
        }  
    } else {
        ret = pthread_create(&my_tid, NULL, (void *)thread_2, NULL);  
        if (ret != 0)  {  
            printf("Create pthread error!\n");  
            return -1;  
        }  
    }

    pthread_join(my_tid, NULL);  
    return 0;
}



