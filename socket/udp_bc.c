
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
#include <pthread.h>

#define DEFAULT_PORT 	8200

int listen_port = DEFAULT_PORT;
int peer_port = DEFAULT_PORT + 100;
  
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

void* listen_task(void *param)
{
    int sock;
    char msg_buf[1024] = {0};
    struct sockaddr_in local_addr;  
    const int opt = 1;  

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)   
    {     
		printf("socket failed \n");
        return NULL;  
    }     

    // 绑定地址  
    bzero(&local_addr, sizeof(struct sockaddr_in));  
    local_addr.sin_family = AF_INET;  
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    local_addr.sin_port = htons(listen_port);  
    if (bind(sock,(struct sockaddr *)&(local_addr), sizeof(struct sockaddr_in)) == -1)   
    {     
        printf("bind failed \n");
        return NULL;  
    }  
 
    while(1)  
    {  
	    int len = sizeof(struct sockaddr_in);  
	    char smsg[100] = {0};  

		//从广播地址接受消息  
        int ret = recvfrom(sock, smsg, 100, 0, (struct sockaddr*)&local_addr, (socklen_t*)&len);  
        if (ret <= 0)  
        {  
            printf("recvfrom failed \n");
        }  
        else  
        {         
            printf("recv: %s\n", smsg);     
        }  
		sleep(1);
    }  
	
    close(sock);
	return NULL;  
}

void send_task(char *app_name)
{
	int sock = -1;	
    struct sockaddr_in peer_addr;  
    const int opt = 1;  
    int ret;  

	//setvbuf(stdout, NULL, _IONBF, 0);   
    //fflush(stdout);	

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)   
    {     
		printf("socket failed \n");
        return ;  
    }     

    //设置该套接字为广播类型，  
    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, (char *)&opt, sizeof(opt));  
    if (ret == -1)  
    {  
		printf("setsockopt failed \n");
        return ;  
    }  
 
    bzero(&peer_addr, sizeof(struct sockaddr_in));  
    peer_addr.sin_family = AF_INET;  
    peer_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);  
    peer_addr.sin_port = htons(peer_port);  
 
    while(1)  
    {  
        sleep(3);  
        ret = sendto(sock, app_name, strlen(app_name), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr));  
        if (ret < 0)  
        {  
			printf("sendto failed \n");
        }  
    }  	
	close(sock);
}

 
int main(int argc, char **argv)
{
	pthread_t tid;

	if (argc < 4) {
		printf("usage: %s <app_name> <local_port> <peer_port> \n", argv[0]);
		return 0;
	}

	listen_port = atoi(argv[2]);
	peer_port = atoi(argv[3]);
	
	pthread_create(&tid, NULL, listen_task, 0);
	send_task(argv[1]);
    
    return 0;
}

