#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

sem_t *rf_cfg_sem = NULL; 

int main(int argc, char **argv)
{
    int temp_val, ret;

    if (argc < 2) {
        printf("%s <take|give> \n", argv[0]);
        return 0;
    }

    rf_cfg_sem = sem_open("rf_cfg_sem", O_CREAT, O_RDWR, 1);
    if (rf_cfg_sem != SEM_FAILED) {
        ret = sem_getvalue(rf_cfg_sem, &temp_val);
        printf("sem_getvalue ret %d, val %d \n", ret, temp_val);
    } else {
        printf("sem_open failed \n");
        return 0;
    }

    if (strncasecmp(argv[1], "take", strlen(argv[1])) == 0) {
        ret = sem_wait(rf_cfg_sem);
        printf("sem_wait ret %d \n", ret);
    } else if (strncasecmp(argv[1], "give", strlen(argv[1])) == 0) {
        ret = sem_post(rf_cfg_sem);
        printf("sem_post ret %d \n", ret);
    }

    return 0;
}

