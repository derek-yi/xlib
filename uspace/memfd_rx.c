
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

typedef struct data{
    char temperature_1[8];
    char temperature_2[8];
    char temperature_3[8];
    char temperature_4[8];
    char temperature_5[8];
}Data;

static int recv_fd(int socket, int n)
{
    int fd;
    struct msghdr msg={0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n*sizeof(int))],data;
    struct iovec io={.iov_base=&data,.iov_len=1};

    memset(buf, 0, sizeof(buf));
    msg.msg_iov         = &io;
    msg.msg_iovlen      = 1;
    msg.msg_control     = buf;
    msg.msg_controllen  = sizeof(buf);
    if (recvmsg(socket, &msg, 0) < 0) {
        perror("[Failed to receive message]");
        return -1;
    }
    
    cmsg = CMSG_FIRSTHDR(&msg);
    memcpy(&fd, (int*)CMSG_DATA(cmsg), n*sizeof(int));
    
    return fd;
}

int main(int argc,char **argv)
{
    int rfd,cfd,fd;
    struct sockaddr_un addr;

    rfd = socket(AF_UNIX,SOCK_STREAM,0);
    if (rfd < 0) {
        perror("[socket error]");
        return -1;
    }
    
    if(unlink("/home/jacky/01.socket")==-1 && errno !=ENOENT){
        perror("[Removing socket file failed]");
        return -1;
    }
    
    memset(&addr,0,sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/home/jacky/Work/Machine_room_monitoring/1.socket", sizeof(addr.sun_path)-1);

    if (bind(rfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))==-1) {
        perror("[Failed to bind socket]");
        return -1;
    }
    
    if (listen(rfd, 5) == -1) {
        perror("[Failed to listen socket]");
        return -1;
    }
    printf("listening......\n");
    
    cfd = accept(rfd, NULL, NULL);
    if (cfd == -1) {
        perror("[Failed to accept incoming connection]");
    }
    printf("cfd:%d\n", cfd);
    
    fd  = recv_fd(cfd, 1);
    if (fd < 0) {
        return -1;
    }

    Data *ptr = (Data *)mmap(NULL, sizeof(Data), PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == NULL) {
        perror("[mmap fd error]");
        return -1;
    }
    
    while (1){
        printf("%s\n", ptr->temperature_1);
        printf("%s\n", ptr->temperature_2);
        printf("%s\n", ptr->temperature_3);
        printf("%s\n", ptr->temperature_4);
        printf("%s\n", ptr->temperature_5);
        sleep(3);
    }
    
    close(fd);
    close(cfd);
    return 0;
}

