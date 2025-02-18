
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
    char sendbuf[256];
    char recvbuf[1024] = {0};
    struct sockaddr_in srv_addr;

    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family      = AF_INET;
	srv_addr.sin_port        = htons(9011);
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	bind(sock, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr));
	
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(udp_port);
    srv_addr.sin_addr.s_addr = inet_addr(srv_ip);
    
	sprintf(sendbuf, "123456");
    ret = sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    printf("send ret %d\n", ret);
    
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

