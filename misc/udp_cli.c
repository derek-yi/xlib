
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
 
#define ERR_EXIT(m) \
do \
{ \
    perror(m); \
    exit(EXIT_FAILURE); \
} while(0)
 
void echo_cli(char *srv_ip, int udp_port)
{
    int sock;
    int ret;
    int index = 0;
    char sendbuf[256] = {0};
    char recvbuf[256] = {0};
    struct sockaddr_in servaddr;

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");
	
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(udp_port);
    servaddr.sin_addr.s_addr = inet_addr(srv_ip);
    
    //while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    while (1)
    {
    	//if (memcmp(sendbuf, "quit", 4) == 0) break;
    	sprintf(sendbuf, "INDEX %d\r\n", index++);
		
        printf("send to srv: %s\n", sendbuf);
        sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        #if 0
        ret = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            ERR_EXIT("recvfrom");
        }
        printf("recv: %s\n", recvbuf);
        #endif
        sleep(1);
    }
    
    close(sock);
}
 
int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("usage: %s <srv_ip> <udp_port> \n", argv[0]);
		return 0;
	}
	
    echo_cli(argv[1], atoi(argv[2]));
    
    return 0;
}

