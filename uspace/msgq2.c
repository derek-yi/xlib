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
    char msgtext[256];
} PRIV_MSG_INFO;

int send_task2(void)  
{
    PRIV_MSG_INFO sndmsg;
    //int var_len = 10;
    
    msg_qid = msgget((key_t)1234, 0666 | IPC_CREAT);  
    if (msg_qid == -1) {
        printf("msgget error\n");
        exit(0);
    }

    for(;;)
    {
        memset(&sndmsg, 0, sizeof(PRIV_MSG_INFO));
        sndmsg.msgtype = 10;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        //memset(&sndmsg, 0x31, var_len); //not support var_len
        //if (++var_len > 100) var_len = 10;
        
        //if (msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, var_len + 8, 0)==-1) {
        if (msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1) {
            printf("msgsnd error\n");
            exit(0);
        }
        
        sndmsg.msgtype = 20;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if (msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1) {
            printf("msgsnd error\n");
            exit(0);
        }
        
        sleep(2);
    }
}

int recv_task2(void)  
{
    PRIV_MSG_INFO rcvmsg;

    msg_qid = msgget((key_t)1234, 0666 | IPC_CREAT);  
    if (msg_qid == -1) {
        printf("msgget error\n");
        exit(0);
    }

    for(;;)
    {
        if (msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 10, 0) == -1) {
            printf("msgrcv error\n");
            exit(0);
        }
        
        printf("recv msg: %s\n", rcvmsg.msgtext);
        //printf("recv msg len %ld\n", strlen(rcvmsg.msgtext));
    }
}

int create_task(int param)  
{  
    pthread_t thread_id;  
    int ret;  

    if (param) {
        ret = pthread_create(&thread_id, NULL, (void *)recv_task2, NULL);  
    } else {
        ret = pthread_create(&thread_id, NULL, (void *)send_task2, NULL);  
    }
    
    if (ret != 0)  {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    pthread_join(thread_id, NULL);  
    return 0;  
}  
 

int main(int argc, char **argv)
{
    int ret;
    
    if (argc < 2) {
        printf("usage: %s <tx:0|rx:1> \n", argv[0]);  
        return 0;
    }

    ret = create_task(atoi(argv[1]));
    
    return ret;
}




