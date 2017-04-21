


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
int execl(const char *pathname, const char *arg, …);
int execlp(const char *filename,conxt char *arg, …);
int execle(const char *pathname,conxt char *arg, …,char *const envp[ ])；
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

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif


#if T_DESC("global", 1)


int main(int argc, char **argv)
{
     pid_t pid;
     int status;
     
     if((pid = fork()) < 0)
     {
         status = -1;
     }
     else if(pid == 0)
     {
         printf("child: \n");
         // 执行/bin目录下的ls, 第一参数为程序名ls, 第二个参数为"-al", 第三个参数为"/etc/passwd"
         execl("/bin/ls", "ls", "-al", "/etc/passwd", (char *) 0);
         
         //system("ls -a");
         _exit(127);
     }
     else
     {
         printf("father: 11\n");
         while(waitpid(pid, &status, 0) < 0)
         {
             if(errno != EINTR)
             {
                 status = -1;
                 break;
             }
         }
         printf("father: 22\n");
     }
     
     return status;
 } 

#endif



