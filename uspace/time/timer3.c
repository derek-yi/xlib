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

typedef int (* timer_cb)(void *param);


typedef void (* lib_callback)(union sigval);

int vos_create_timer(timer_t *ret_tid, int interval, int repeat, timer_cb callback, void *param)
{
	timer_t timerid;
	struct sigevent evp;
	struct itimerspec it;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = param; 
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = (lib_callback)callback;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
        *ret_tid = 0; //0 as gift 
		return -1;
	}
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长
	it.it_value.tv_sec = interval/1000;
	it.it_value.tv_nsec = (interval%1000)*1000*1000;
	if ( repeat ) {
		it.it_interval.tv_sec  = it.it_value.tv_sec;
		it.it_interval.tv_nsec = it.it_value.tv_nsec;
	} else {
		it.it_interval.tv_sec  = 0;
		it.it_interval.tv_nsec = 0;
	}
    
    timer_settime(timerid, 0, &it, NULL);  //not care result
    *ret_tid = timerid;
    return 0;
}

int vos_delete_timer(timer_t timerid)
{
    if (timerid == 0) return -1; //0 cause segment fault
	return timer_delete(timerid);
}

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

int timer_sleep = 0;

int interval = 10;

int my_timer_cb(char *param)
{
    printf("my_timer_cb(%d) param %s \n", gettid(), param);
    if (timer_sleep) sleep(timer_sleep);
    printf("my_timer_cb(%d) done \n", gettid());
}

int main(int argc, char **argv)
{
    timer_t my_timer;

    if (argc < 3) {
        printf("%s <interval> <sleep> \n", argv[0]);
        return 0;
    }

    interval = atoi(argv[1])*1000;
    timer_sleep = atoi(argv[2]);
    vos_create_timer(&my_timer, interval, 1, my_timer_cb, "12345");
    while (1) sleep(1);
    return 0;
}

