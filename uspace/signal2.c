
/*
#include <signal.h>
void (*signal(int signumber,void((*func)(int)))(int);
int sigaction(int sigunumberm, const structr sigaction *act,struct sigaction *oldact);

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
int kill (pid_t pid, int signumber);
int raise (int signumber);
unsigned int alarm (unsigned int seconds);

*/

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

void sig_handler(int signumber)
{
    printf("catch signal %d", signumber);
}
void terminate_handler(int signumber)
{
    printf("catch signal %d", signumber);
    sleep(5);
    //exit(0);
}

int tu1_proc(void)
{
    char buffer1[128];
    struct sigaction act;
    sigset_t blockset;
    
    if(signal(SIGINT, &sig_handler)==SIG_ERR)
    {
        printf("failed to register signal handler for SIGINT!\n");
    }
    
    if(signal(SIGTSTP, &sig_handler)==SIG_ERR)
    {
        printf("failed to register signal handler for SIGTSTP!\n");
    }

    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGALRM, &act, NULL)==-1)
    {
        printf("failed to register signal handler for SIGALRM!\n");
    }

    sigemptyset(&blockset);
    //sigaddset(&blockset, SIGTERM);
    //sigaddset(&blockset, SIGINT);
    act.sa_mask = blockset;
    //act.sa_flags = SA_NODEFER; //中断处理过程中收到信号不按系统缺省处理(内核将不会阻塞该信号)
    act.sa_flags = SA_RESETHAND; //中断处理过程中收到信号按系统缺省处理
    act.sa_sigaction = &terminate_handler;
    if(sigaction(SIGTERM, &act, NULL)==-1)
    {
        printf("failed to register signal handler for SIGTERM!\n");
    }
    
    printf("pid of This Process: %d", getpid());
    alarm(10); // 10 seconds
    
    for( ; ; )
    {
        printf("\n input(sigint|sigtstp|sigterm): ");
        fgets(buffer1, sizeof(buffer1), stdin);
        if(strncmp(buffer1, "sigint", 6)==0) {
            printf("\n test SIGINT \n");
            raise(SIGINT);
        }
        if(strncmp(buffer1, "sigtstp", 7)==0) {
            printf("\n test SIGTSTP \n");
            kill(0, SIGTSTP);
        }
        if(strncmp(buffer1, "sigterm", 7)==0) {
            printf("\n test SIGTERM \n");
            kill(0, SIGTERM);
        }
    }

    return 0;
}

#endif

#if T_DESC("TU2", 1)

int tu2_proc(void)
{
    char buffer1[128];
    sigset_t blockset, pending;
    int pendingcount;
    
    alarm(10); // 10 seconds

    if(signal(SIGALRM, &sig_handler)==SIG_ERR)
    {
        printf("failed to register signal handler for SIGALRM!\n");
    }
    
    for( ; ; )
    {
        sigaddset(&blockset, SIGALRM);
        sigprocmask(SIG_BLOCK, &blockset, NULL);
        
        fgets(buffer1, sizeof(buffer1), stdin);
        printf("input: %s\n", buffer1);
        
        sigpending(&pending);
        pendingcount=0;
        if(sigismember(&pending,SIGINT))
            pendingcount++;
        if(sigismember(&pending,SIGALRM))
            pendingcount++;
        
        if(pendingcount)
        {
            printf("there are %d signals pending.\n", pendingcount);
        }
    }


    return 0;
}
  
#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- signal");
    printf("\n   2 -- pending");
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
    if (tu == 2) ret = tu2_proc();
    
    return ret;
}
#endif



