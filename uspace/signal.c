


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

    printf("pid of This Process: %d", getpid());

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

#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- signal send test");
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
    
    return ret;
}
#endif



