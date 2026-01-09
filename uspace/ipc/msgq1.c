#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>


/*
#include <sys/type.h>
#include <sys/ipc.h>
#include <sys/msg.h>
int msgget(key_t key, int flag);
int msgsnd(int msqid, const void *ptr, size_t nbytes, int flag);
int msgrcv(int msqid, void *ptr, size_t nbytes, long type, int flag);
int msgctl(int msqid, int cmd, struct mspid_ds *buf);
*/


int msg_qid;
int msg_qid2;

typedef struct msgbuf
{
    long msgtype;
    char msgtext[128];
} PRIV_MSG_INFO;

int send_task(void)  
{
    PRIV_MSG_INFO sndmsg;
    int serial_num = 0;
	int ret;

    for(;;)
    {
        sprintf(sndmsg.msgtext, "sn %d", serial_num++);
        sndmsg.msgtype = serial_num%10;

		ret = msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sndmsg.msgtype*10, 0);
        if (ret == -1) {
            printf("send msg_qid error %d \n", ret);
            exit(254);
        }

		sndmsg.msgtype = serial_num%2;
		ret = msgsnd(msg_qid2, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0);
        if (ret == -1) {
            printf("send msg_qid2 error %d \n", ret);
            //exit(254);
        }
        
        sleep(5);
    }
}

int recv_task(void)  
{
	int ret;
    PRIV_MSG_INFO rcvmsg;

    for(;;)
    {
    	ret = msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 0, 0);
        if ( ret == -1) {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv_task recv msg(%d): type %ld, %s\n", ret, rcvmsg.msgtype, rcvmsg.msgtext);
    }
}

int recv_task2(void)  
{
	int ret;
    PRIV_MSG_INFO rcvmsg;

    for(;;)
    {
    	//If msgtyp is 0, then the first message in the queue is read.
    	//If msgtyp is greater than 0, then the first message in the queue of type msgtyp is read, unless MSG_EXCEPT was specified in msgflg
    	ret = msgrcv(msg_qid2, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 1, 0);
        if ( ret == -1) {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv_task2 recv msg(%d): type %ld, %s\n", ret, rcvmsg.msgtype, rcvmsg.msgtext);
    }
}

int create_task(void)  
{  
    pthread_t id_1,id_2,id_3;  
    int i,ret;  

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if (msg_qid == -1) {
        printf("msgget error\n");
        exit(254);
    }

    msg_qid2 = msgget(IPC_PRIVATE, 0666);
    if (msg_qid2 == -1) {
        printf("msgget error\n");
        exit(254);
    }

    ret = pthread_create(&id_1, NULL, (void *)send_task, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)recv_task, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    ret = pthread_create(&id_3, NULL, (void *)recv_task2, NULL);  
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  
    pthread_join(id_3, NULL);  
    
    msgctl(msg_qid, IPC_RMID, 0);
    msgctl(msg_qid2, IPC_RMID, 0);

    return 0;  
}  


//ipcs -q
//ipcrm -q qid
int main(int argc, char **argv)
{
    create_task();
    return 0;
}



