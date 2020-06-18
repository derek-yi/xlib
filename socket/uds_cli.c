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

#define BUFFER_SIZE     512

const char *filename = "uds-tmp";

int main(int argc, char **argv)
{
    struct sockaddr_un un;
    int sock_fd, i;
    char buffer[BUFFER_SIZE];

    if(argc < 2){
        printf("usage: %s <s1> <..>\n", argv[0]);
        return -1;
    }
    
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, filename);
    sock_fd = socket(AF_UNIX,SOCK_STREAM,0);
    if(sock_fd < 0){
        printf("Request socket failed\n");
        return -1;
    }
    
    if(connect(sock_fd,(struct sockaddr *)&un,sizeof(un)) < 0){
        printf("connect socket failed\n");
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    for(i = 1; i < argc; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    }
    
    send(sock_fd, buffer, strlen(buffer), 0);

    close(sock_fd);
    return 0;
}



