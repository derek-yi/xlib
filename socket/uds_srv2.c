
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/un.h> 

 
const char *uds_name = "uds-tmp";
  
#define ERR_EXIT(m) \
do { \
    perror(m); \
    exit(EXIT_FAILURE); \
} while (0)

void echo_server(int sock)
{
    char recvbuf[1024] = {0};
    struct sockaddr_un peeraddr;
    socklen_t peerlen;
    int n;
    
    while (1)
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        peerlen = sizeof(peeraddr);
        //peerlen = strlen(peeraddr.sun_path) + sizeof(peeraddr.sun_family); 
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
        if (n <= 0)
        {
            if (errno == EINTR)
                continue;
            
            ERR_EXIT("recvfrom error");
        }
        else if(n > 0)
        {
            printf("recv buf: %s\n", recvbuf);
            sendto(sock, recvbuf, n, 0, (struct sockaddr *)&peeraddr, peerlen);
            printf("send buf: %s\n", recvbuf);
        }
    }
    close(sock);
}
 
int main(void)
{
    int sockfd = 0;  
    struct sockaddr_un addr;  
    
    unlink(uds_name);    
    addr.sun_family = AF_UNIX;  
    strcpy(addr.sun_path, uds_name);  

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

    echo_server(sockfd);
    
    return 0;
}

