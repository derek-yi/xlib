
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("TU1", 1)

char *flock_name1 = "/tmp/flock1";
char *flock_name2 = "/tmp/flock2";
int flock_fd1;
int flock_fd2;

int read_lock(int fd)
{
    struct flock lock;  
    lock.l_start = 0;  
    lock.l_len = 0x10;  
    lock.l_type = F_RDLCK;  
    lock.l_whence = SEEK_SET;  
    int result = fcntl(fd, F_SETLK, &lock);  
    if(result<0){  
        perror("lockR:");  
    }  
    return result; 
}

int read_lock_wait(int fd)
{
    struct flock lock;  
    lock.l_start = 0;  
    lock.l_len = 0x10;  
    lock.l_type = F_RDLCK;  
    lock.l_whence = SEEK_SET;  
    return fcntl(fd, F_SETLKW, &lock);  
}

int write_lock(int fd)
{
    struct flock lock;  
    lock.l_start = 0;  
    lock.l_len = 0x10;  
    lock.l_type = F_WRLCK;  
    lock.l_whence = SEEK_SET;  
    return fcntl(fd, F_SETLK, &lock);
}

int write_lock_wait(int fd)
{
    struct flock lock;  
    lock.l_start = 0;  
    lock.l_len = 0x10;  
    lock.l_type = F_WRLCK;  
    lock.l_whence = SEEK_SET;  
    return fcntl(fd, F_SETLKW, &lock);  
}

int file_unlock(int fd)
{
    struct flock lock;  
    lock.l_start = 0;  
    lock.l_len = 0x10;  
    lock.l_type = F_UNLCK;  
    lock.l_whence = SEEK_SET;  
    return fcntl(fd, F_SETLK, &lock);  
}

void thread_11(void)  
{  
    int i;  
    for(i=0; i<10; i++)  {
        write_lock(flock_fd1);
        printf("This is pthread_1.\n");  
        sleep(1);  
        printf("pthread_1 sleep ok.\n");
        file_unlock(flock_fd2);
    }  
    pthread_exit(0);  
}  
  
void thread_12(void)  
{  
    int i;  
    for(i=0; i<10; i++) {
        write_lock(flock_fd2);
        printf("This is pthread_2.\n");  
        sleep(1);  
        printf("pthread_2 sleep ok.\n");
        file_unlock(flock_fd1);
    }  
    pthread_exit(0);  
}  

int tu1_proc(int argc, char **argv)  
{  
    pthread_t id_1,id_2;  
    int fd,ret;  
    int param;

    if (argc < 2) return 1;
    param = atoi(argv[1]);

    flock_fd1 = open(flock_name1, O_RDWR| O_CREAT, 0600);
    if (flock_fd1 < 0) return -1;
    
    flock_fd2 = open(flock_name2, O_RDWR| O_CREAT, 0600);
    if (flock_fd2 < 0) return -1;

    if (!param) {
        ret = pthread_create(&id_1, NULL, (void *)thread_11, NULL);  
    } else {
        ret = pthread_create(&id_2, NULL, (void *)thread_12, NULL);  
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

    close(flock_fd1);
    close(flock_fd2);
    
    return 0;  
}  

#endif

#if T_DESC("global", 1)
void usage()
{
    printf("\n Usage: <cmd> <tu> <p1> <...>");
    printf("\n   1 -- sem between thread");
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
    if (tu == 1) ret = tu1_proc(argc - 1, &argv[1]);
    
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
1, how to compile 
gcc -o flock.out flock.c -lpthread

*/
#endif

