


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

int send_task(void)  
{
    PRIV_MSG_INFO sndmsg;

    for(;;)
    {
        sndmsg.msgtype++;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1) {
            printf("msgsnd error\n");
            exit(254);
        }
        sleep(5);
    }
}

int recv_task(void)  
{
    PRIV_MSG_INFO rcvmsg;

    for(;;)
    {
        if(msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 0, 0) == -1) {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv msg: %s\n", rcvmsg.msgtext);
    }
}

int create_task(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if(msg_qid == -1) {
        printf("msgget error\n");
        exit(254);
    }

    ret = pthread_create(&id_1, NULL, (void *)send_task, NULL);  
    if(ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)recv_task, NULL);  
    if(ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  
    msgctl(msg_qid, IPC_RMID, 0);

    return 0;  
}  



int main(int argc, char **argv)
{
    create_task();
    return 0;
}



