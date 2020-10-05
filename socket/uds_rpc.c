

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>



#define BUFFER_SIZE         512


#if xxx

char* server_file = "server.sock";
char* client_file = "client.sock";


int create_client(void)
{
    struct sockaddr_un srv_addr;
    struct sockaddr_un cli_addr;
    int sock_fd;
    char buffer[BUFFER_SIZE];

    sock_fd = socket(AF_UNIX, SOCK_DGRAM,0);
    if(sock_fd < 0){
        perror("Request socket failed\n");
        return -1;
    }

    cli_addr.sun_family = AF_UNIX;
    unlink(client_file);
    strcpy(cli_addr.sun_path,client_file);
    if(bind(sock_fd, &cli_addr, sizeof(cli_addr)) <0 ){
        perror("bind failed!\n");
        return -1;
    }

    srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, server_file);
    socklen_t len = sizeof(srv_addr);
    while(1) {
        char *p = "Hello, uds server";
        int ssize = sendto(sock_fd, p, strlen(p), 0, &srv_addr, len);
        if (ssize < 0) {
            perror("sendto");
            break;
        }

        int size = recvfrom(sock_fd, buffer,sizeof(buffer),0, &srv_addr, &len);
        if (size < 0) {
            perror("recv");
            break;
        } else {
            printf("client recv: %s\n", buffer);
        }
        sleep(3);
    }

    close(sock_fd);
    return 0;
}


int create_server()
{
    int sock_fd;
    struct sockaddr_un srv_addr;
    
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sock_fd < 0){
        perror("Request socket failed!\n");
        return -1;
    }
    
    srv_addr.sun_family = AF_UNIX;
    unlink(server_file);
    strcpy(srv_addr.sun_path, server_file);
    if(bind(sock_fd, &srv_addr, sizeof(srv_addr)) <0 ){
        perror("bind failed!\n");
        return -1;
    }
    
    while(1){
        struct sockaddr_un cli_addr;
        socklen_t len = sizeof(cli_addr);
        char buffer[BUFFER_SIZE];
        
        bzero(buffer, BUFFER_SIZE);
        int ret = recvfrom(sock_fd, buffer, sizeof(buffer), 0, &cli_addr, &len);
        if(ret < 0){
            printf("recv failed\n");
        } else {
            printf("server recv: %s\n", buffer);
        }
        
        char *p = "OK,I got id!";
        int ssize = sendto(sock_fd, p, strlen(p), 0, &cli_addr, len);
        if(ssize < 0){
            printf("send failed\n");
        } 
    }
    
    close(sock_fd);
    return 0;
}
#endif

int sock_fd = -1;
char my_uds_name[64];

void uds_echo(void)  
{  

    while(1){
        struct sockaddr_un peer_addr;
        socklen_t len = sizeof(peer_addr);
        char buffer[BUFFER_SIZE];
        
        bzero(buffer, BUFFER_SIZE);
        int ret = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer_addr, &len);
        if(ret < 0){
            printf("recv failed\n");
        } else {
            printf("%s: recv: %s\n", my_uds_name, buffer);
        }
        
        char *p = "OK,I got id!";
        int ssize = sendto(sock_fd, p, strlen(p), 0, (struct sockaddr *)&peer_addr, len);
        if(ssize < 0){
            printf("send failed\n");
        } 
    }
}  

int create_uds_main(char *local_name, char *peer_name)
{
    pthread_t unused_id;
    struct sockaddr_un local_addr;
    struct sockaddr_un peer_addr;
    
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sock_fd < 0){
        perror("Request socket failed!\n");
        return -1;
    }

    sprintf(my_uds_name, "%s", local_name);
    local_addr.sun_family = AF_UNIX;
    unlink(local_name);
    strcpy(local_addr.sun_path, local_name);
    if(bind(sock_fd, &local_addr, sizeof(local_addr)) <0 ){
        perror("bind failed!\n");
        return -1;
    }

    int ret = pthread_create(&unused_id, NULL, (void *)uds_echo, NULL);  
    if(ret != 0)  {  
        perror("Create pthread error!\n");  
        return -1;  
    }  

    peer_addr.sun_family = AF_UNIX;
    strcpy(peer_addr.sun_path, peer_name);
    socklen_t len = sizeof(peer_addr);
    while(1) {
        char buffer[BUFFER_SIZE];

        sprintf(buffer, "hello, %s", peer_name);
        int ssize = sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&peer_addr, len);
        if (ssize < 0) {
            perror("sendto");
            sleep(1);
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int size = recvfrom(sock_fd, buffer,sizeof(buffer),0, (struct sockaddr *)&peer_addr, &len);
        if (size < 0) {
            perror("recv");
            break;
        } else {
            printf("%s: recv: %s\n", local_name, buffer);
        }
        sleep(3);
    }
    
    close(sock_fd);
    return 0;
}

//https://blog.csdn.net/briblue/article/details/89350869    
int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s <local> <peer>\n");
        return 0;
    }

    create_uds_main(argv[1], argv[2]);
    return 0;
}
