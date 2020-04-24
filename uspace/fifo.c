

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

int tu1_proc(void)
{
    int fifo_fd;
    int ret;
    char buffer[128];
    
    mkfifo("my_fifo", 0644);
    fifo_fd = open("my_fifo", O_WRONLY);
    printf("fifo_fd: %d\n", fifo_fd);

    for( ; ; )
    {
        printf("\n input write buffer(exit to break): ");
        fgets(buffer, sizeof(buffer), stdin);

        if(strncmp(buffer, "exit", 4) == 0) break;

        ret = write(fifo_fd, buffer, strlen(buffer) + 1);
        printf("write: %d\n", ret);
    }

    close(fifo_fd);
    unlink("my_fifo"); 
    
    return 0;
}

#endif

#if T_DESC("TU2", 1)

int tu2_proc(void)
{
    int fifo_fd;
    int ret;
    char buffer[128];
    
    //mkfifo("my_fifo", 0644);
    fifo_fd = open("my_fifo", O_RDONLY);
    printf("fifo_fd: %d\n", fifo_fd);

    for( ; ; )
    {
        ret = read(fifo_fd, buffer, 128);
        if(ret > 0) {
            printf("read: %s\n", buffer);
        }
    }

    return 0;
}

#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- create write fifo");
    printf("\n   2 -- create read fifo");
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


