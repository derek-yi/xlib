

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void DieWithUserMessage(const char *msg, const char *detail) {
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}
 
void DieWithSystemMessage(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv){
 
    if ( argc < 3) {
        DieWithUserMessage("Parameter(s)", "<IP> <PORT> [<MESSAGE>]");
    }
 
    // Create local socket
    int sock;
    if ( ( sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
        DieWithSystemMessage("socket() failed.");
    } 
 
    int servPort = atoi(argv[2]);
 
    // Create server address
    struct sockaddr_in servAddr;
    memset( &servAddr, 0, sizeof(servAddr) );
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    inet_pton(AF_INET, argv[1], &servAddr.sin_addr.s_addr);
 
    // Connect to server
    if ( (connect( sock, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0  ) {
        DieWithSystemMessage("connect() failed.");
    }
 
    // Send Message to server
    ssize_t sentByteLen = send( sock, argv[3], sizeof(argv[3]), 0);
    if ( sentByteLen > 0 ) {
        printf("send: %s\tbytes:%d\n", argv[3], sentByteLen);
    } else {
        DieWithSystemMessage("send() failed");
    }
    
    // Receive Message from server
    char recvBuf[sentByteLen];
    ssize_t recvByteLen = recv( sock, recvBuf, sentByteLen, 0);
    if ( recvByteLen > 0 ) {
        printf("recv: %s\tbytes:%d\n", recvBuf, recvByteLen);
    } else {
        DieWithSystemMessage("recv() failed");
    }
 
    close(sock);
    
    printf("end!\n");
}


