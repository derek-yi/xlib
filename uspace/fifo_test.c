#define _GNU_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>

__thread int my_var = 6;
int task_run = 1;

int t1_run_cnt = 0;
int t2_run_cnt = 0;

int test_param = 2000;

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

int do_sth(int seed)
{
	int i, sum = 0;
	for (i = 0; i < seed; i++) {
		sum += i;
		sum = sum^seed;
	}
	for (i = 0; i < seed; i++) {
		sum += i*2;
		sum = sum^seed;
	}
	for (i = 0; i < seed; i++) {
		sum += i*3;
		sum = sum^seed;
	}
	return sum;
}

int my_nanosleep(int nsec)
{
    struct timespec ts = {
        0, nsec
    };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

int nano_delay_test()
{
	struct timeval tv_start, tv_end;
	unsigned long i, delay, nTimeTest;

	for (i = 0, delay = 1; i < 10; i++) {
		gettimeofday(&tv_start, NULL);
		my_nanosleep(delay);
		gettimeofday(&tv_end, NULL);
		nTimeTest = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec;	
		printf("delay %ld(ns), real %lu(us) \r\n", delay, nTimeTest);
		delay = delay*10;
	}
	return 0;
}

void thread_1(void)  
{  
    struct sched_param param;
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);

    param.sched_priority = 50;
    sched_setscheduler(0, SCHED_FIFO, &param);
	printf("thread_1: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_1: getscheduler %d \r\n", sched_getscheduler(0));
    while (task_run)  {
        //printf("thread_1: my_var=%d addr=0x%x \n", my_var, &my_var);
        my_var += do_sth(20000);
		t1_run_cnt++;
	
		//no give up: 2000
		//give up: 10000
        my_nanosleep(test_param); 
        //sched_yield();
    }  
	printf("thread_1: run_cnt %d \r\n", t1_run_cnt);
    //pthread_exit(0);  
}  

void thread_2(void)  
{  
    struct sched_param param;
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);

    param.sched_priority = 50;
    sched_setscheduler(0, SCHED_FIFO, &param);
	printf("thread_2: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_2: getscheduler %d \r\n", sched_getscheduler(0));
	
    while (task_run)  {
        //printf("thread_2: my_var=%d addr=0x%x \n", my_var, &my_var);
        my_var += do_sth(10000);
		t2_run_cnt++;
	
        my_nanosleep(test_param); 
        //sched_yield();
    }  

	printf("thread_2: run_cnt %d \r\n", t2_run_cnt);
    //pthread_exit(0);  
}  

int pthread_test1(void)  
{  
    pthread_t id_1,id_2;  
    int ret;  
    unsigned long stack_size;
    pthread_attr_t attr;
    struct sched_param param;
	int policy;
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
	
    param.sched_priority = 80;
    sched_setscheduler(0, SCHED_FIFO, &param);

    printf("SCHED_FIFO: Max %u, Min %u\n", sched_get_priority_max(SCHED_FIFO), sched_get_priority_min(SCHED_FIFO));
    printf("SCHED_RR: Max %u, Min %u\n", sched_get_priority_max(SCHED_RR), sched_get_priority_min(SCHED_RR));
    printf("SCHED_OTHER: Max %u, Min %u\n", sched_get_priority_max(SCHED_OTHER), sched_get_priority_min(SCHED_OTHER));

	nano_delay_test();
	//nano_delay_test();
	sleep(1);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//param.sched_priority = 50;
	//pthread_attr_setschedparam(&attr, &param);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
    ret = pthread_create(&id_1, &attr, (void *)thread_1, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

	//sleep(1);
    ret = pthread_create(&id_2, &attr, (void *)thread_2, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

	printf("main: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("main: getscheduler %d \r\n", sched_getscheduler(0));
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  
    
    return 0;  
}  

void handle_signal(int signal)
{
    if ( (signal == SIGINT) || (signal == SIGTERM) ) {
        task_run = 0;
		printf("t1_run_cnt %d, t2_run_cnt %d \r\n", t1_run_cnt, t2_run_cnt);
		sleep(1);
		exit(0);
    }
}

int main(int argc, char **argv)
{
    struct sigaction sa;

	if (argc > 1) {
		test_param = atoi(argv[1]);
	}

    sa.sa_handler = &handle_signal;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

    pthread_test1();
	sleep(1);
    return 0;
}





