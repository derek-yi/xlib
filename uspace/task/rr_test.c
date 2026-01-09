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

int sched_mode = 1;
int run_cpu = 4;
int task1_prio = 30;
int task2_prio = 20;

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

int vos_bind_to_cpu(int cpu_id)
{
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
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

void thread_1(void)  
{  
    struct sched_param param;

    vos_bind_to_cpu(run_cpu);
    param.sched_priority = task1_prio;
    if (sched_mode == 0) sched_setscheduler(0, SCHED_RR, &param);
    else if (sched_mode == 1) sched_setscheduler(0, SCHED_FIFO, &param);
    else sched_setscheduler(0, SCHED_OTHER, &param);
	printf("thread_1: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_1: getscheduler %d \r\n", sched_getscheduler(0));
    sleep(1);

    while (task_run)  {
        //printf("thread_1: my_var=%d addr=0x%x \n", my_var, &my_var);
        my_var += do_sth(10000);
		t1_run_cnt++;
    }  

    pthread_exit(0);  
}  

void thread_2(void)  
{  
    struct sched_param param;

	vos_bind_to_cpu(run_cpu);
    param.sched_priority = task2_prio;
    if (sched_mode == 0) sched_setscheduler(0, SCHED_RR, &param);
    else if (sched_mode == 1) sched_setscheduler(0, SCHED_FIFO, &param);
    else sched_setscheduler(0, SCHED_OTHER, &param);
	printf("thread_2: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("thread_2: getscheduler %d \r\n", sched_getscheduler(0));
    sleep(1);
	
    while (task_run)  {
        //printf("thread_2: my_var=%d addr=0x%x \n", my_var, &my_var);
        my_var += do_sth(10000);
		t2_run_cnt++;
	
    }  

    pthread_exit(0);  
}  

int pthread_test1(void)  
{  
    pthread_t id_1,id_2;  
    int ret;  
    unsigned long stack_size;
    pthread_attr_t attr;
    struct sched_param param;
	int policy;

    printf("SCHED_FIFO: Max %u, Min %u\n", sched_get_priority_max(SCHED_FIFO), sched_get_priority_min(SCHED_FIFO));
    printf("SCHED_RR: Max %u, Min %u\n", sched_get_priority_max(SCHED_RR), sched_get_priority_min(SCHED_RR));
    printf("SCHED_OTHER: Max %u, Min %u\n", sched_get_priority_max(SCHED_OTHER), sched_get_priority_min(SCHED_OTHER));

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    ret = pthread_create(&id_1, &attr, (void *)thread_1, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    ret = pthread_create(&id_2, &attr, (void *)thread_2, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

	printf("main: ppid=%d pid=%d tid=%d self=0x%x\n", getppid(), getpid(), gettid(), (int)pthread_self());
    printf("main: getscheduler %d \r\n", sched_getscheduler(0));
    return 0;  
}  

void handle_signal(int signal)
{
    if ( (signal == SIGINT) || (signal == SIGTERM) ) {
        task_run = 0;
		sleep(1);
		exit(0);
    }
}

int main(int argc, char **argv)
{
    struct sigaction sa;

	if (argc < 4) {
        printf("%s <cpu> <prio_1> <prio_2> <mode:0-rr,1-fifo,2-other>\n", argv[0]);
        return 0;
	}

    run_cpu = atoi(argv[1]);
    task1_prio = atoi(argv[2]);
    task2_prio = atoi(argv[3]);
    if (argc > 4) sched_mode = atoi(argv[4]);
    
    sa.sa_handler = &handle_signal;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

    pthread_test1();
	while (1) {
        sleep(2);
        printf("t1_run_cnt %d, t2_run_cnt %d \r\n", t1_run_cnt, t2_run_cnt);
    }
    return 0;
}





