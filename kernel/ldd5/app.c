
#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/mman.h>  

int thread_1(void)  
{
    int uio_fd;
    int c, ret;

    uio_fd  = open("/dev/uio0", O_RDWR); ///sys/class/uio/uio0/name: uio_irq
    if(uio_fd < 0) {
        fprintf(stderr, "open: %s\n", strerror(errno));
        exit(-1);
    }

    while (1) {
        ret = read(uio_fd, &c, sizeof(int));
        if (ret > 0) {
            printf("current event count %d\n", c);
            c = 1;
            write(uio_fd, &c, sizeof(int));
        }
    }

    close(uio_fd);

    return 0;
}  

#define IO_CMD_LEN      256  
char kdev_io_buf[IO_CMD_LEN] = {0};

int main()
{
    pthread_t my_thread;
    int char_fd, ret, i;

    char_fd  = open("/dev/kdev", O_RDWR);
    if(char_fd < 0) {
        fprintf(stderr, "open: %s\n", strerror(errno));
        exit(-1);
    }

    ret = pthread_create(&my_thread, NULL, (void *)thread_1, NULL);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  
    sleep(1);

    for(i = 0; i < 10; i++){
        sprintf(kdev_io_buf, "sendsig");
        ret = ioctl(char_fd, 0, kdev_io_buf);
        printf("ioctl: ret=%d rdata:%s\n", ret, kdev_io_buf);
        sleep(1);
    }

    close(char_fd);

    return 0;
}

