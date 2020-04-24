
#include <stdio.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

#define IO_CMD_LEN      256  
char kdev_io_buf[IO_CMD_LEN] = {0};

void signal_handler(int signo)
{
    printf("signal_handler: %d\n", signo);
}

pid_t gettid(void)
{  
    return syscall(SYS_gettid);  
} 

int main()
{
    int fd;
    int ret = 0;

    signal(SIGUSR1, (void *)signal_handler);
    printf("main: pid=%d tid=%d \n", getpid(), gettid());
    
    fd = open("/dev/plat_led", O_RDWR);
    if( fd < 0 ) {
        printf("open plat_led WRONG£¡\n");
        return 0;
    }

    sprintf(kdev_io_buf, "sendsig");
    ret = ioctl(fd, 0, kdev_io_buf);
    printf("ioctl: ret=%d rdata:%s\n", ret, kdev_io_buf);

    sprintf(kdev_io_buf, "showres");
    ret += ioctl(fd, 0, kdev_io_buf);
    printf("ioctl: ret=%d rdata:%s\n", ret, kdev_io_buf);
    
    close(fd);
    return 0;
}

