



#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*
#include <sys/types.h>
#include <sys/star.h>
int mkfifo(const char *pathame, mode_t mode);
int mknod(char *pathname, mode_t mode, dev_t dev);

mkfifo a=rw FIFO_TEST
mknod FIFO_TEST p

*/

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

int write_buffer(int fd, const void *buf, int count)
{
const void *pts=buf;
int status=0, n;
if(count<0)
return(-1);
while(status!=count)
{
if(n=write(fd, pts+status,count-status)==-1)
{
printf(¡°error, failed to write! \n¡±);
exit(254);
}
else if(n<0) {
return (n);
status+=n;
}
return(status);
}
}

int read_buffer(int fd, void *buf, int count)
{
int status;
int count=0;
while(count<maxlen-1)
{
if(status=read_buffer(socket, buf_count, 1))<1)
{
return ¨C1;
}
if(buf[count]==delim)
{
buf[count]=0;
return 0;
}
count++;
}
buf[count]=0;
return 0;
}


int readnlstring(int socket, char *buf, int amxlen, char delim)
    {
    void *pts=buf;
    int status=0, n;
    if(count<0)
    return(-1);
    while(status!=count)
    {if(n=read(fd, pts+status,count-status)==-1)
{
printf(¡°error, failed to write! \n¡±);
exit(254);
}
else if(n<0) {
return (n);
status+=n;
}
return(status);
}
}


void parent(char *argv[ ])
{
char buffer[100];
int fd;
close(0);
if(mkfifo(¡°my-fifo¡±,0600)==-1)
{
printf(¡°error, failed to creat my-fifo!\n¡±);
exit(254);
}
printf(¡°the server is listening on my-fifo.\n¡±);
if(fd=open(¡°my-fifo¡±, O_RDONLY)==-1)
{
printf(¡°error, failed to open my-fifo!\n¡±);
exit(254);
}
printf(Client has connected. \n¡±);
while(readnlstring(fd, buffer, sizeof(buffer))>=0)
{
printf(¡°received message: %s \n¡±, buffer);
}
printf(¡°No more data; parent exiting. \n¡±);
if(close(fd)==-1)
{
printf(¡°error, close failed!¡±);
exit(254);
}
unlink(¡°my-fifo¡±);
}
void child(char *argv[ ])
{
int fd;
char buffer[100];
if(fd=open(¡°my-fifo¡±, O_WRONLY)==-1)
{
printf(¡°error, failed to open my-fifo!\n¡±);
exit(254);
}
printf(¡°The client is ready. Enter messages, or Ctrl+D when done. \n¡±);
while(fgets(buffer, sizeof(buffer), stdin)!=NULL)
{
write_buffer(fd, buffer, strlen(buffer));
}
printf(¡°client exiting. \n¡±);
if(close(fd)==-1)
{
printf(¡°error, close failed!¡±);
exit(254);
}
}    


#endif

#if T_DESC("TU2", 1)

#endif

#if T_DESC("global", 1)

void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- sem between thread");
    printf("\n   2 -- sem between process, need su mode");
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


