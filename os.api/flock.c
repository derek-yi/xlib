
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif

#if T_DESC("fcntl", 1)

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
#endif

#if T_DESC("flock", 1)

#endif


#if T_DESC("tu", 1)

void thread_11(void)  
{  
    int i;  
    for(i=0; i<100; i++)  {
        write_lock(flock_fd1);
        printf("This is pthread_111...");  
        sleep(2);  
        printf("111.\n");
        file_unlock(flock_fd1);
        sleep(1);
    }  
    pthread_exit(0);  
}  
  
void thread_12(void)  
{  
    int i;  
    for(i=0; i<100; i++) {
        write_lock(flock_fd2);
        printf("This is pthread_222...");  
        sleep(2);  
        printf("222.\n");
        file_unlock(flock_fd1);
        sleep(1);  
    }  
    pthread_exit(0);  
}  

void thread_21(void)  
{  
    int i;  
    for(i=0; i<100; i++)  {
        flock(flock_fd1, LOCK_EX);
        printf("This is pthread_111...");  
        sleep(2);  
        printf("111.\n");
        flock(flock_fd1, LOCK_UN);
        sleep(1);  
    }  
    pthread_exit(0);  
}  
  
void thread_22(void)  
{  
    int i;  
    for(i=0; i<100; i++) {
        flock(flock_fd1, LOCK_EX);
        printf("This is pthread_222...");  
        sleep(2);  
        printf("222.\n");
        flock(flock_fd1, LOCK_UN);
        sleep(1);  
    }  
    pthread_exit(0);  
}  

int tu1_proc(int tu_id)  
{  
    pthread_t id_1,id_2;  
    int fd,ret;  
    int param;

    flock_fd1 = open(flock_name1, O_RDWR| O_CREAT, 0600);
    if (flock_fd1 < 0) return -1;
    
    flock_fd2 = open(flock_name2, O_RDWR| O_CREAT, 0600);
    if (flock_fd2 < 0) return -1;

    if (tu_id == 1) {
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
    if (tu_id == 1) {
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
    printf("\n   1 -- create task 1");
    printf("\n   2 -- create task 2");
    printf("\n");
}

int main(int argc, char **argv)
{
    int ret;
    int tu_id;
    
    if(argc < 2) {
        usage();
        return 0;
    }

    tu_id = atoi(argv[1]);
    if (tu_id < 1 || tu_id > 2)  {
        usage();
        return 0;
    }
    
    ret = tu1_proc(tu_id);
    return ret;
}
#endif

#if T_DESC("readme", 1)
/*
1, how to compile 
gcc -o flock.out flock.c -lpthread

*/
#endif

