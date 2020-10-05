

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



int srv_fd = -1;
int api_fd = -1;
char my_uds_name[64];

void uds_echo(void)  
{  

    while(1){
        struct sockaddr_un peer_addr;
        socklen_t len = sizeof(peer_addr);
        static int serial_num = 0;
        char buffer[BUFFER_SIZE];
        
        bzero(buffer, BUFFER_SIZE);
        int ret = recvfrom(srv_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer_addr, &len);
        if(ret < 0){
            printf("recv failed\n");
        } else {
            printf("%s.srv: recv: %s\n", my_uds_name, buffer);
        }
        
        sprintf(buffer, "echo sn %d", serial_num++);
        int ssize = sendto(srv_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&peer_addr, len);
        if(ssize < 0){
            printf("send failed\n");
        } else {
            printf("%s.srv: send %s(%d)\n", my_uds_name, buffer, ssize);
        }
    }
}  

int create_uds_main(char *local_name, char *peer_name)
{
    pthread_t unused_id;
    struct sockaddr_un local_addr;
    struct sockaddr_un peer_addr;
    
    api_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(api_fd < 0){
        perror("Request socket failed!\n");
        return -1;
    }
    
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, my_uds_name);
    strcat(local_addr.sun_path, ".api");
    unlink(local_addr.sun_path);
    if (bind(api_fd, &local_addr, sizeof(local_addr)) <0 ) {
        perror("bind failed!\n");
        return -1;
    }

    srv_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (srv_fd < 0) {
        perror("Request socket failed!\n");
        return -1;
    }

    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, my_uds_name);
    strcat(local_addr.sun_path, ".srv");
    unlink(local_addr.sun_path);
    if (bind(srv_fd, &local_addr, sizeof(local_addr)) <0 ) {
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
    strcat(peer_addr.sun_path, ".srv");
    socklen_t len = sizeof(peer_addr);
    while(1) {
        char buffer[BUFFER_SIZE];
        static int serial_num = 0;

        sprintf(buffer, "hello %s, sn %d", peer_name, serial_num++);
        int ssize = sendto(api_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&peer_addr, len);
        if (ssize < 0) {
            perror("sendto");
            sleep(1);
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int size = recvfrom(api_fd, buffer,sizeof(buffer),0, (struct sockaddr *)&peer_addr, &len);
        if (size < 0) {
            perror("recv");
            break;
        } else {
            printf("%s.api: recv: %s\n", local_name, buffer);
        }
        sleep(3);
    }

    close(srv_fd);
    close(api_fd);
    return 0;
}
/*

aa.srv
bind aa.srv
recv bb.api
send bb.api

aa.api
bind aa.api
send to bb.srv
recv bb.srv

bb.srv
bind bb.src
recv aa.api
send aa.api

bb.api
bind bb.api
send to aa.srv
recv aa.srv

*/

//https://blog.csdn.net/briblue/article/details/89350869    
int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s <local> <peer>\n");
        return 0;
    }

    sprintf(my_uds_name, "%s", argv[1]);
    create_uds_main(argv[1], argv[2]);
    return 0;
}
