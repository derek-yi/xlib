
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define NETLINK_TEST        30
#define MSG_LEN             256
#define MAX_PLOAD           256

typedef struct _user_msg_info
{
    struct nlmsghdr hdr;
    char  msg[MSG_LEN];
} user_msg_info;

int skfd;
int my_nl_pid;

int send_msg(int dst_pid, char *msg)
{
    int ret;
    struct nlmsghdr *nlh = NULL;
    struct sockaddr_nl saddr, daddr;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = dst_pid;
    daddr.nl_groups = 0;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
    memset(nlh, 0, sizeof(struct nlmsghdr));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_type = 22;
    nlh->nlmsg_seq = 11;
    nlh->nlmsg_pid = my_nl_pid;

    memcpy(NLMSG_DATA(nlh), msg, strlen(msg));
    ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
    if(!ret)
    {
        perror("sendto error");
    }

    free((void *)nlh);
    return 0;
}

void *rx_task(void *param)
{
    int ret;
    user_msg_info u_info;

    while(1) {
        memset(&u_info, 0, sizeof(u_info));
        ret = recv(skfd, &u_info, sizeof(user_msg_info), 0);
        if(!ret)
        {
            perror("recv error");
            sleep(3);
            continue;
        }

        printf("rx_task: %s \r\n", u_info.msg);    
        printf("  pid=%d len=%d type=%d seq=%d \r\n", u_info.hdr.nlmsg_pid, u_info.hdr.nlmsg_len, u_info.hdr.nlmsg_type, u_info.hdr.nlmsg_seq);    
    }

    return NULL;
}

int create_listen_task(int local_pid)
{
    int ret;
    struct sockaddr_nl saddr;
    pthread_t id_1;

    /* 创建NETLINK socket */
    skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if(skfd == -1)
    {
        perror("socket error");
        return -1;
    }

    my_nl_pid = local_pid;
    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK; //AF_NETLINK
    saddr.nl_pid = local_pid;  //端口号(port ID) 
    saddr.nl_groups = 0;
    if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
    {
        perror("bind error");
        close(skfd);
        return -1;
    }

    pthread_create(&id_1, NULL, (void *)rx_task, NULL);  
    return 0;
}

int main(int argc, char **argv)
{
    int dst_id;
    char msg_buf[256];
    
    if (argc < 2) {
        printf("usage: %s <local-pid> \r\n", argv[0]);
        return 0;
    }

    create_listen_task(atoi(argv[1]));
    while(1) {
        printf("\r\n input dst-pid and string: ");
        scanf("%d %s", &dst_id, msg_buf);
        send_msg(dst_id, msg_buf);
    }
    
    return 0;
}
