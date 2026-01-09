#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

void sig_handler(int signo)
{
	printf("timer_signal param %d\n", signo);
}

int main()
{
	timer_t timerid;
	struct sigevent evp;

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("fail to sigaction");
		exit(-1);
	}

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_signo = SIGUSR1;
	evp.sigev_notify = SIGEV_SIGNAL;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
		perror("fail to timer_create");
		exit(-1);
	}

	struct itimerspec it;
	it.it_interval.tv_sec = 3;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 3;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, 0) == -1) {
		perror("fail to timer_settime");
		exit(-1);
	}

	while(1)sleep(1);
	return 0;
}

