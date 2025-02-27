#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
static const int MAXPENDING = 5;
 
int rx_wait = 1;

int rx_max = 1000;

int total_cnt = 0;
  
void DieWithSystemMessage(const char *msg) 
{
    perror(msg);
    exit(1);
}

void HandleTCPClient(int clntSocket)
{
    char buffer[9000]; //risk
    ssize_t numBytesRcvd = 0;

    while (1) {
        if (rx_max) {
            numBytesRcvd = recv(clntSocket, buffer, rx_max, 0);
            if ( numBytesRcvd < 0 ) DieWithSystemMessage("recv() failed");
            total_cnt += numBytesRcvd;
        }
        printf("recv %d bytes, total %d \n", numBytesRcvd, total_cnt);
        sleep(rx_wait);
    }

    close(clntSocket);
}
  
int main(int argc, char *argv[]) 
{
    if (argc < 2) {
          DieWithSystemMessage("usage: cmd <port> [<rx_max> <wait>]");
    }
 
    in_port_t servPort = atoi(argv[1]);
    int servSock;

    if (argc > 2) rx_max  = atoi(argv[2]);
    if (argc > 3) rx_wait = atoi(argv[3]);
    if((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
        DieWithSystemMessage("Socket() failed");
    }
 
    struct sockaddr_in servAddr; 
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family =  AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);
    
    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 )
        DieWithSystemMessage("bind() failed");
 
    if (listen(servSock, MAXPENDING) < 0 )
        DieWithSystemMessage("listen() failed");
 
    for (;;) { 
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);
        
        // Wait for a client to connect
        int clntSock = accept(servSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if ( clntSock < 0 )
            DieWithSystemMessage("accept() failed");
        
        // clntSock is connected to a client!
        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL )
            printf("Handling client %s port %d\n", clntName, ntohs(clntAddr.sin_port));
        else
            printf("Unable to get client address\n");
 
        HandleTCPClient(clntSock);
    }
}
 

