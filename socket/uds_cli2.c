
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/un.h> 


const char *serv_name = "uds-tmp";
const char *local_name = "local-name";
 
#define ERR_EXIT(m) \
do \
{ \
    perror(m); \
    exit(EXIT_FAILURE); \
} while(0)
 
void echo_cli(int sockfd)
{
    struct sockaddr_un servaddr;  
    bzero(&servaddr,sizeof(servaddr));  
    
    servaddr.sun_family = AF_UNIX;  
    strcpy(servaddr.sun_path, serv_name);  
    
    int ret;
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        struct sockaddr_un peeraddr;
        socklen_t peerlen;

        printf("send buf: %s\n", sendbuf);
        sendto(sockfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        
        ret = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        //peerlen = sizeof(peeraddr);
        //ret = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            ERR_EXIT("recvfrom");
        }
        printf("recv buf: %s\n", recvbuf);
        
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }
    
    close(sockfd);
}
 
int main(void)
{
    int sockfd = 0;  
    struct sockaddr_un addr;  
    
    unlink(local_name);    
    addr.sun_family = AF_UNIX;  
    strcpy(addr.sun_path, local_name);  

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);  
    if(sockfd < 0 )  
    {  
        perror("socket error");  
        exit(-1);  
    }  

    //unsigned int len = strlen(addr.sun_path) + sizeof(addr.sun_family); 
    if(bind(sockfd,(struct sockaddr *)&addr, sizeof(addr)) < 0)  
    {  
        perror("bind error");  
        close(sockfd);  
        exit(-1);  
    }  
    printf("Bind is ok\n");

    echo_cli(sockfd);
    
    return 0;
}

