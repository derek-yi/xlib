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

    fd = open("/sys/module/vmem/parameters/dev_num", O_RDONLY, S_IRUSR);    //读取模块设备数量参数dev_num
    if (fd == -1) {
        printf("Failed to open parameters for dev_num.\n");
        return -1;
    }
    read(fd, tmp, 10);
    dev_num = atoi(tmp);    //获取dev_num
    if (dev_num > 1) {        //如果dev_num = 1默认不需要参数
        if (argc == 2)
            input_num = atoi(*(++argv));
        else {
            printf("please input the dev_num between 0 and %d.\n", dev_num - 1);
            return -1;
        }
        if (**argv < '0' || **argv > '9' || dev_num <= input_num) {        //第一个参数第一位不为0~9返回错误信息
            printf("please input the dev_num between 0 and %d.\n", dev_num - 1);
            return -1;
        }
    }

    snprintf(path, 30 + 1, "/dev/vmem%d", input_num);        //获取设备路径名字

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
    epfd = epoll_create(1);    //创建epoll
    if (epfd < 0) {
        printf("epoll_create failed.\n");
        return -1;
    }
    ev_virtualmem.events = EPOLLIN | EPOLLPRI;        //epoll触发事件
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_virtualmem) < 0) {    //增加这个事件
        printf("epfd_ctl add failed.\n");
        return -1;
    }

    ret = epoll_wait(epfd, &ev_virtualmem, 1, 5000);        //等待事件，调用驱动poll
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
    signal(SIGIO, signalio_handler);        //声明signalio_handler信号处理函数
    fcntl(fd, F_SETOWN, getpid());        //获取程序pid
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC);    //添加FASYNC标志
    while (1)
        sleep(100);
#endif

    close(fd);
    return 0;
}