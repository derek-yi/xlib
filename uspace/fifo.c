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

int write_proc(void)
{
    int fifo_fd;
    int ret;
    char buffer[128];
    
    ret = mkfifo("my_fifo", 0644);
    printf("mkfifo: %d\n", ret);

    fifo_fd = open("my_fifo", O_WRONLY);
    printf("fifo_fd: %d\n", fifo_fd);

    for( ; ; )
    {
        printf("\n input write buffer(exit to break): ");
        fgets(buffer, sizeof(buffer), stdin);

        if(strncmp(buffer, "exit", 4) == 0) break;

        ret = write(fifo_fd, buffer, strlen(buffer));
        printf("write: %d\n", ret);
    }

    close(fifo_fd);
    unlink("my_fifo"); 
    
    return 0;
}

int read_proc(void)
{
    int fifo_fd;
    int ret;
    char buffer[128];
    
    ret = mkfifo("my_fifo", 0644);
    printf("mkfifo: %d\n", ret);
    
    fifo_fd = open("my_fifo", O_RDONLY);
    printf("fifo_fd: %d\n", fifo_fd);

    for( ; ; )
    {
        memset(buffer, 0, sizeof(buffer));
        ret = read(fifo_fd, buffer, 128);
        if(ret > 0) {
            printf("\nread: %s\n", buffer);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
    }
    else if (pid > 0)
    {
        printf("parent pid %d\n", getpid());
        write_proc();
    }
    else
    {
        printf("child pid %d\n", getpid());
        read_proc();
    }
    
    return 0;
}


