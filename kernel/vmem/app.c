#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/stat.h>

//#undef EPOLL
#define EPOLL

#define MEM_CLEAR           _IO('V', 1)
#define MEM_FULL            _IOW('V', 2, unsigned char)

char buff[4096];
int fd;

void signalio_handler(int signum)
{
    printf("receive a signal \n");
    read(fd, buff, 4096);
    printf("%s \n", buff);
}

int main(int argc, char const **argv)
{
    int oflags, input_num = 0, dev_num;
    char tmp[10];
    char path[30];
#ifdef EPOLL
    int ret;
    struct epoll_event ev_virtualmem;
    int epfd;
#endif

    fd = open("/sys/module/vmem/parameters/dev_num", O_RDONLY, S_IRUSR);
    if (fd == -1) {
        printf("Failed to open parameters for dev_num.\n");
        return -1;
    }
    
    read(fd, tmp, 10);
    dev_num = atoi(tmp);
    if (dev_num > 1) {
        if (argc == 2)
            input_num = atoi(*(++argv));
        else {
            printf("please input the dev_num between 0 and %d.\n", dev_num - 1);
            return -1;
        }
        if (**argv < '0' || **argv > '9' || dev_num <= input_num) {
            printf("please input the dev_num between 0 and %d.\n", dev_num - 1);
            return -1;
        }
    }

    snprintf(path, 30 + 1, "/dev/vmem%d", input_num);

    fd = open(path, O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("Failed to open /dev/virtualmem%d.\n", input_num);
        return -1;
    }
    printf("open /dev/virtualmem%d success.\n", input_num);

    if (ioctl(fd, MEM_FULL, 0xFF) < 0)
        printf("ioctl command = 0x%lx failed.\n", MEM_FULL);

    if (ioctl(fd, MEM_CLEAR, 0xFF) < 0)
        printf("ioctl command = 0x%x failed.\n", MEM_CLEAR);
    
#ifdef EPOLL
    epfd = epoll_create(1);
    if (epfd < 0) {
        printf("epoll_create failed.\n");
        return -1;
    }
    ev_virtualmem.events = EPOLLIN | EPOLLPRI;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_virtualmem) < 0) {
        printf("epfd_ctl add failed.\n");
        return -1;
    }

    ret = epoll_wait(epfd, &ev_virtualmem, 1, 5000);
    if (ret < 0)
        printf("epoll_wait failed.\n");
    else if (!ret)
        printf("no data input in virtualmem%d\n", input_num);
    else {
        printf("receive data.\n");
        read(fd, buff, 4096);
        printf("%s \n", buff);
    }
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev_virtualmem)) {
        printf("epfd_ctl delete failed.\n");
        return -1;
    }
#else
    signal(SIGIO, signalio_handler);
    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC); 
    while (1)
        sleep(100);
#endif

    close(fd);
    return 0;
}
