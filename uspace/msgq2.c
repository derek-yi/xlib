
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

typedef struct msgbuf
{
    long msgtype;
    char msgtext[128];
} PRIV_MSG_INFO;

int send_task2(void)  
{
    PRIV_MSG_INFO sndmsg;

    msg_qid = msgget((key_t)1234, 0666 | IPC_CREAT);  
    if(msg_qid == -1) {
        printf("msgget error\n");
        exit(254);
    }

    for(;;)
    {
        sndmsg.msgtype = 10;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1) {
            printf("msgsnd error\n");
            exit(254);
        }
        
        sndmsg.msgtype = 20;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1) {
            printf("msgsnd error\n");
            exit(254);
        }
        sleep(3);
    }
}

int recv_task2(void)  
{
    PRIV_MSG_INFO rcvmsg;

    msg_qid = msgget((key_t)1234, 0666 | IPC_CREAT);  
    if(msg_qid == -1) {
        printf("msgget error\n");
        exit(254);
    }

    for(;;)
    {
        if(msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 10, 0) == -1) {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv msg: %s\n", rcvmsg.msgtext);
    }
}

int create_task(int param)  
{  
    pthread_t thread_id;  
    int ret;  

    if (!param) {
        ret = pthread_create(&thread_id, NULL, (void *)send_task2, NULL);  
    } else {
        ret = pthread_create(&thread_id, NULL, (void *)recv_task2, NULL);  
    }
    if(ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(thread_id, NULL);  
    return 0;  
}  
 

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        printf("usage: %s <0|1> \n", argv[0]);  
        return 0;
    }

    ret = create_task(atoi(argv[1]));
    
    return ret;
}




