


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

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

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
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1)
        {
            printf("msgsnd error\n");
            exit(254);
        }
        sleep(3);
    }
}

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

int tu1_proc(void)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if(msg_qid == -1)
    {
        printf("msgget error\n");
        exit(254);
    }

    ret = pthread_create(&id_1, NULL, (void *)send_task, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    ret = pthread_create(&id_2, NULL, (void *)recv_task, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    /*等待线程结束*/  
    pthread_join(id_1, NULL);  
    pthread_join(id_2, NULL);  

    msgctl(msg_qid, IPC_RMID, 0);

    return 0;  
}  


#endif

#if T_DESC("TU2", 1)

int send_task2(void)  
{
    PRIV_MSG_INFO sndmsg;

    msg_qid = msgget((key_t)1234, 0666 | IPC_CREAT);  
    if(msg_qid == -1)
    {
        printf("msgget error\n");
        exit(254);
    }

    for(;;)
    {
        sndmsg.msgtype=10;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1)
        {
            printf("msgsnd error\n");
            exit(254);
        }
        
        sndmsg.msgtype=20;
        sprintf(sndmsg.msgtext, "type %ld", sndmsg.msgtype);
        if(msgsnd(msg_qid, (PRIV_MSG_INFO *)&sndmsg, sizeof(PRIV_MSG_INFO), 0)==-1)
        {
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
    if(msg_qid == -1)
    {
        printf("msgget error\n");
        exit(254);
    }

    for(;;)
    {
        if(msgrcv(msg_qid, (PRIV_MSG_INFO *)&rcvmsg, sizeof(PRIV_MSG_INFO), 10, 0) == -1)
        {
            printf("msgrcv error\n");
            exit(254);
        }
        printf("recv msg: %s\n", rcvmsg.msgtext);
    }
}

int tu2_proc(int argc, char **argv)  
{  
    pthread_t id_1,id_2;  
    int i,ret;  
    int param;

    if (argc < 2) return 1;
    param = atoi(argv[1]);

    if (!param) {
        ret = pthread_create(&id_1, NULL, (void *)send_task2, NULL);  
    } else {
        ret = pthread_create(&id_2, NULL, (void *)recv_task2, NULL);  
    }
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    
    /*等待线程结束*/  
    if (!param) {
        pthread_join(id_1, NULL);  
    } else {
        pthread_join(id_2, NULL);  
    }
    
    return 0;  
}  
  
#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- msgQ between thread");
    printf("\n   2 -- msgQ between process");
    printf("\n     => P1: 0 - create pid 0; 1 - create pid 1");
    printf("\n");
}

int main(int argc, char **argv)
{
    int ret;
    
    if(argc < 2) {
        usage();
        return 0;
    }

    int tu = atoi(argv[1]);
    if (tu == 1) ret = tu1_proc();
    if (tu == 2) ret = tu2_proc(argc - 1, &argv[1]);
    
    return ret;
}
#endif




