

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

#ifndef SIG_TIMER

int msg_qid;

pthread_t recv_thread;

typedef struct msgbuf
{
    long msgtype;
    char msgtext[128];
} PRIV_MSG_INFO;

int recv_task(void)  
{
    PRIV_MSG_INFO rcvmsg;

    for(;;)
    {
        if(msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 0, 0) == -1)
        {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv msg: %s\n", rcvmsg.msgtext);
    }
}

void timer_thread(union sigval v)
{
    PRIV_MSG_INFO sndmsg;
    static int msgtype = 100;

	printf("timer_thread param %d\n", v.sival_int);

    sndmsg.msgtype = msgtype++;
    sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
    msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0);
}


int main()
{
    int ret;
	timer_t timerid;
	struct sigevent evp;

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if(msg_qid == -1) {
        printf("msgget error\n");
        exit(254);
    }

    ret = pthread_create(&recv_thread, NULL, (void *)recv_task, NULL);  
    if(ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    } 
    
	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 111; // param for timer_thread
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread;
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

    pthread_join(recv_thread, NULL);  
    msgctl(msg_qid, IPC_RMID, 0);
	return 0;
}

#else

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
#endif


