#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <sys/syscall.h>


pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

void timer_test_cb(union sigval v)
{
    static int timer_cnt = 0;
    
    timer_cnt++;
    printf("timer_test_cb(%d) param %d, timer_cnt %d\n", gettid(), v.sival_int, timer_cnt);
}

int main()
{
    int ret;
	timer_t timerid;
	struct sigevent evp;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 111; // param for timer_thread
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_test_cb;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
		perror("fail to timer_create");
		exit(-1);
	}
	
	struct itimerspec it;
	it.it_interval.tv_sec = 3;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 3;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, NULL) == -1) {
		perror("fail to timer_settime");
		exit(-1);
	}

    while (1) sleep(1);
	return 0;
}


