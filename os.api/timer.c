

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

#if 1

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

    // ��ʱ���ص�����Ӧ�ü򵥴���
	printf("timer_thread function! %d\n", v.sival_int);

    sndmsg.msgtype++;
    sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
    msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0);
}


// int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
// clockid��CLOCK_REALTIME, CLOCK_MONOTONIC��CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID
// evp--��Ż���ֵ�ĵ�ַ,�ṹ��Ա˵���˶�ʱ�����ڵ�֪ͨ��ʽ�ʹ���ʽ��
// timerid--��ʱ����ʶ��

// int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);
// timerid--��ʱ����ʶ
// flags--0��ʾ���ʱ�䣬1��ʾ����ʱ��
// new_value--��ʱ�����³�ʼֵ�ͼ�����������it
// old_value--ȡֵͨ��Ϊ0�������ĸ�������ΪNULL,����ΪNULL���򷵻ض�ʱ����ǰһ��ֵ

int main()
{
    int ret;
	timer_t timerid;
	struct sigevent evp;

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if(msg_qid == -1)
    {
        printf("msgget error\n");
        exit(254);
    }

    ret = pthread_create(&recv_thread, NULL, (void *)recv_task, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    } 
    
	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 111; // param for timer_thread
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		perror("fail to timer_create");
		exit(-1);
	}
	
	//��һ�μ��it.it_value��ô��,�Ժ�ÿ�ζ���it.it_interval��ô��,����˵it.it_value��0��ʱ���װ��it.it_interval��ֵ
	struct itimerspec it;
	it.it_interval.tv_sec = 3;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 3;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		perror("fail to timer_settime");
		exit(-1);
	}

    /*�ȴ��߳̽���*/  
    pthread_join(recv_thread, NULL);  
    msgctl(msg_qid, IPC_RMID, 0);

	return 0;
}

#endif

#if xxx

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

void sig_handler(int signo)
{
	printf("timer_signal function! %d\n", signo);
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
	if (sigaction(SIGUSR1, &act, NULL) == -1)
	{
		perror("fail to sigaction");
		exit(-1);
	}

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_signo = SIGUSR1;
	evp.sigev_notify = SIGEV_SIGNAL;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		perror("fail to timer_create");
		exit(-1);
	}

	struct itimerspec it;
	it.it_interval.tv_sec = 3;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 3;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, 0) == -1)
	{
		perror("fail to timer_settime");
		exit(-1);
	}

	pause();

	return 0;
}
#endif


