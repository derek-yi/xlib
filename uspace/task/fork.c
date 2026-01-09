#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
#include <unistd.h>
int execl(const char *pathname, const char *arg, бн);
int execlp(const char *filename,conxt char *arg, бн);
int execle(const char *pathname,conxt char *arg, бн,char *const envp[ ])г╗
int execv(const char *pathname, char *const argv[ ]);
int execvp(const char *filename, char *const argv[ ]);
int execve(const char *pathname, char *const argv[ ],char *const envp[ ]);

#include <stdlib.h>
int system(const char *cmdstring);

#include <sched.h>
int__clone(int (*fn)(void *arg),void *child_stack,int flags,void *arg);

#include <stdlib.h>
void exit(int status);
int atexit(void (*function)(void));
int on_exit(void(*function)(int, void*), void *arg);
void abort(void);

#include <unistd,h>
void _exit(int status);

#include <assert.h>
void assert(int expression);

*/


int main(int argc, char **argv)
{
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        status = -1;
    } else if(pid == 0) {
        printf("child: start \n");
        
        //execl("/bin/ls", "ls", "-al", "/etc/passwd", (char *) 0);
        system("ls -al /etc/passwd");

        //execl do not run next
        printf("child: end \n");
        //_exit(10);
    } else {
        printf("father: start \n");
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
        printf("status: %d \n", status);
        printf("father: end \n");
    }

    return status;
} 




