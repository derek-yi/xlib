
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MCAST_PORT    50001
#define MCAST_ADDR    "239.0.0.1"

#define error_exit(_errmsg_)    error(EXIT_FAILURE, errno, _errmsg_)

#define BUFF_SIZE    1024

int main()
{
    int sockfd;
    struct sockaddr_in mcastaddr;
    char *buff = NULL;
    int nbytes;
    time_t time_sec;
    
    /**创建用户数据包套接字**/
    if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0))) 
        error_exit("socket");
        
    /**指定接收方地址为组播地址**/
    mcastaddr.sin_family = AF_INET;
    mcastaddr.sin_port = htons(MCAST_PORT);
    mcastaddr.sin_addr.s_addr = inet_addr(MCAST_ADDR);
    
    /**连接到组播地址**/
    if (-1 == connect(sockfd, (struct sockaddr *)&mcastaddr, sizeof(mcastaddr)))
        error_exit("bind");

    time(&time_sec);
    while (2)  {
        sleep(1);
        time_sec ++;
        buff = ctime(&time_sec);
        printf("%s", buff);
        
        /**数据发送**/
        if (-1 == send(sockfd, buff, strlen(buff), 0))
            error_exit("send");
    }
    close(sockfd);

    return 0;
}

