




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
    printf("\r\ncatch signal %d \r\n", signumber);
}

void terminate_handler(int signumber)
{
    printf("\r\ncatch signal %d \r\n", signumber);
    sleep(3);
    exit(0);
}

int tu1_proc(void)
{
    char buffer1[128];
    struct sigaction act;
    
    if(signal(SIGUSR1, &sig_handler)==SIG_ERR)
    {
        printf("failed to register signal handler for SIGINT!\n");
    }
    
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGUSR2, &act, NULL)==-1)
    {
        printf("failed to register signal handler for SIGALRM!\n");
    }

    printf("pid of This Process: %d \r\n", getpid());
    for( ; ; )
    {
        printf("\n input dest pid: ");
        fgets(buffer1, sizeof(buffer1), stdin);

        if (atoi(buffer1) > 0) {
            kill(atoi(buffer1), SIGUSR1);
            kill(atoi(buffer1), SIGUSR2);
        }
    }

    return 0;
}

int tu2_proc(void)
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
    //act.sa_flags = SA_NODEFER; 
    act.sa_flags = SA_RESETHAND; 
    act.sa_sigaction = terminate_handler;
    if(sigaction(SIGTERM, &act, NULL)==-1)
    {
        printf("failed to register signal handler for SIGTERM!\n");
    }
    
    printf("pid of This Process: %d \r\n", getpid());
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


int tu3_proc(void)
{
    char buffer1[128];
    sigset_t blockset, pending;
    int pendingcount;
    
    alarm(5); // 5 seconds
    if(signal(SIGALRM, &sig_handler)==SIG_ERR)
    {
        printf("failed to register signal handler for SIGALRM! \n");
    }
    
    for( ; ; ) //todo
    {
        fgets(buffer1, sizeof(buffer1), stdin);
        printf("input: %s\n", buffer1);

        sigaddset(&blockset, SIGALRM);
        sigprocmask(SIG_BLOCK, &blockset, NULL);
        
        sigpending(&pending);
        pendingcount = 0;
        if (sigismember(&pending, SIGINT) )
            pendingcount++;
        if (sigismember(&pending, SIGALRM) )
            pendingcount++;
        
        if (pendingcount) 
            printf("there are %d signals pending.\n", pendingcount);
    }


    return 0;
}


int main(int argc, char **argv)
{
    int tuid;
    
    if (argc < 2) {
        printf("usage: %s <tuid> \r\n", argv[0]);
        return 0;
    }

    tuid = atoi(argv[1]);
    if (tuid== 1) tu1_proc();
    if (tuid== 2) tu2_proc();
    if (tuid== 3) tu3_proc();
    
    return 0;
}



