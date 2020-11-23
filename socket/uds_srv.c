
//http://lionoggo.com/2016/10/01/%E8%B0%88Unix%20Domain%20Socket/

#include <stdlib.h>  
#include <stdio.h>  
#include <stddef.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <errno.h>  
#include <string.h>  
#include <unistd.h>  
#include <ctype.h>   
 
#define MAXLINE 80  
 
char *socket_path = "server.socket";  
 
int main(void)  
{  
    struct sockaddr_un server_addr, client_addr;  
    socklen_t client_addr_len;  
    int serverfd; 
    int clientfd;
    int size;  
    char buf[MAXLINE];  
    int i, n;  
    
    // 1. 创建Socket
    if ((serverfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {  
        perror("Socket创建错误");  
        exit(1);  
    }  
    printf("Socket创建完成\n");
    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sun_family = AF_UNIX;  
    strcpy(server_addr.sun_path, socket_path);  
    size = offsetof(struct sockaddr_un, sun_path) + strlen(server_addr.sun_path);  
    unlink(socket_path);  
    
    // 2. 绑定Socket
    if (bind(serverfd, (struct sockaddr *)&server_addr, size) < 0) {  
        perror("Socket绑定错误");  
        exit(1);  
    }  
    printf("Socket绑定成功\n");  
    
    // 3. 监听Socket  
    if (listen(serverfd, 10) < 0) {  
        perror("Socket监听错误");  
        exit(1);          
    }  
    printf("Socket开始监听\n");  
 
    while(1) {  
        client_addr_len = sizeof(client_addr);         
        if ((clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0){  
            perror("连接建立失败");  
            continue;  
        }  
          
        while(1) {  
            n = read(clientfd, buf, sizeof(buf));  
            if (n < 0) {  
                perror("读取客户端数据错误");  
                break;  
            } else if(n == 0) {  
                printf("连接断开\n");  
                break;  
            }  
              
            printf("接受客户端数据: %s", buf);  
 
            for(i = 0; i < n; i++) {  
                buf[i] = toupper(buf[i]);  
            } 
             
            write(clientfd, buf, n);  
            printf("向客户端发送数据: %s", buf); 
        }  
        close(clientfd);  
    }  
    close(serverfd);  
    return 0;  
}

