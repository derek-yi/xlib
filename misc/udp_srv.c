
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>   //time_t
#include <sys/time.h>

#define DEFAULT_PORT 	8200
  
#define ERR_EXIT(m) \
do { \
    perror(m); \
    exit(EXIT_FAILURE); \
} while (0)

void fmt_time_str(char *time_str, int max_len)
{
    struct tm *tp;
    time_t t = time(NULL);
    tp = localtime(&t);
     
    if (!time_str) return ;
    
    snprintf(time_str, max_len, "[%04d-%02d-%02d %02d:%02d:%02d]", 
            tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

void echo_server(int sock)
{
    char msg_buf[1024] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n;
    
    while (1)
    {
        peerlen = sizeof(peeraddr);
        memset(msg_buf, 0, sizeof(msg_buf));
        n = recvfrom(sock, msg_buf, sizeof(msg_buf), 0, (struct sockaddr *)&peeraddr, &peerlen);
        if (n <= 0) {
            if (errno == EINTR)
                continue;
            
            ERR_EXIT("recvfrom error");
        } else if (n > 0) {
            printf("recv: %s\n", msg_buf);

			fmt_time_str(msg_buf, sizeof(msg_buf));
            sendto(sock, msg_buf, strlen(msg_buf) + 1, 0, (struct sockaddr *)&peeraddr, peerlen);
            printf("send back: %s\n", msg_buf);
            sleep(3);
        }
    }
	
    close(sock);
}
 
int main(int argc, char **argv)
{
    int sock;
	int listen_port = DEFAULT_PORT;

	if (argc > 1) {
		listen_port = atoi(argv[1]);
	}
	
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket error");
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(listen_port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    printf("listen port %d\n", listen_port);
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind error");
    
    echo_server(sock);
    
    return 0;
}

