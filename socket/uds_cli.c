
#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
 
#define MAXLINE 80  
 
char *client_path = "client.socket";  
char *server_path = "server.socket";  
 
int main() {  
    struct  sockaddr_un client_addr, server_addr;  
    int len;  
    char buf[100];  
    int sockfd, n;  
 
    // 1. Socket创建
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){  
        perror("Socket创建失败");  
        exit(1);  
    }  
      
    memset(&client_addr, 0, sizeof(client_addr));  
    client_addr.sun_family = AF_UNIX;  
    strcpy(client_addr.sun_path, client_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(client_addr.sun_path);  
    unlink(client_addr.sun_path);  
    
    // 2. Socket绑定
    if (bind(sockfd, (struct sockaddr *)&client_addr, len) < 0) {  
        perror("Socket绑定错误");  
        exit(1);  
    }  
 
    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sun_family = AF_UNIX;  
    strcpy(server_addr.sun_path, server_path);  
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);  
    
    // 3. 连接服务端
    if (connect(sockfd, (struct sockaddr *)&server_addr, len) < 0){  
        perror("连接服务端失败");  
        exit(1);  
    }  
 
    while(fgets(buf, MAXLINE, stdin) != NULL) {    
         write(sockfd, buf, strlen(buf));    
         n = read(sockfd, buf, MAXLINE);    
         if ( n < 0 ) {    
            printf("服务端关闭\n");    
         }else {    
            write(STDOUT_FILENO, buf, n);    
         }    
    }   
    close(sockfd);  
    return 0;  
}



