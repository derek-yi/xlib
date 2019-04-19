
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef uint32
typedef unsigned int uint32;
#endif

#define RC_MAX_CLIENT       16
#define RC_MAX_BUFSIZE      1024

#define RC_MAGIC_NUM        0x5AA51001
#define RC_CMD_REGISTER     0x100
#define RC_CMD_ECHO         0x101

typedef struct
{
    uint32 client_id;
    uint32 state;
    uint32 ip_addr;
    uint32 port;
    
    uint32 rx_bytes;
    uint32 tx_bytes;
}CLIENT_INFO_ST;

typedef struct
{
    uint32 magic;
    uint32 client_id;
    uint32 cmd;
    uint32 subcmd;
    uint32 ret;
    
    uint32 data_len;
    char data[1];
}RC_MSG_INFO;

#if 1
void DieWithSystemMessage(const char *msg) 
{
    perror(msg);
    exit(1);
}

CLIENT_INFO_ST g_client_list[RC_MAX_CLIENT];

void client_msg_task(void *param)
{
    int clntSocket;
    char buffer[RC_MAX_BUFSIZE];    // Buffer for echo string
    RC_MSG_INFO *rc_msg = (RC_MSG_INFO *)buffer;
    
    // Receive message from client.
    ssize_t numBytesRcvd = recv(clntSocket, buffer, RC_MAX_BUFSIZE, 0);
    if ( numBytesRcvd < 0 ) {
        printf("%d: recv() failed \r\n", __LINE__);
        goto CLIENT_CLOSE;
    }

    // Send received string and receive again until end of stream.
    while ( numBytesRcvd > 0 ) {  //0 indicates end of stream.
        // Echo message back to client.
        ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
        if ( numBytesSent < 0 )
            printf("%d: send() failed \r\n", __LINE__);
        else if ( numBytesSent != numBytesRcvd )
            printf("%d: send unexpected number of bytes", __LINE__);
        
        // See if there is more data to receive.
        numBytesRcvd = recv(clntSocket, buffer, RC_MAX_BUFSIZE, 0);
        if ( numBytesRcvd < 0 )
            DieWithSystemMessage("recv() failed.");
    }

    return 0;
}


void HandleTCPClient(int clntSocket)
{
    int clntSocket;
    char buffer[RC_MAX_BUFSIZE];    // Buffer for echo string
    RC_MSG_INFO *rc_msg = (RC_MSG_INFO *)buffer;
    
    // Receive message from client.
    ssize_t numBytesRcvd = recv(clntSocket, buffer, RC_MAX_BUFSIZE, 0);
    if ( numBytesRcvd < 0 ) {
        printf("%d: recv() failed \r\n", __LINE__);
        return ;
    }

    if (rc_msg->magic != RC_MAGIC_NUM) {
        printf("%d: recv() failed \r\n", __LINE__);
        return ;
    }
    
    // Send received string and receive again until end of stream.
    while ( numBytesRcvd > 0 ) {  //0 indicates end of stream.
        // Echo message back to client.
        ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
        if ( numBytesSent < 0 )
            printf("%d: send() failed \r\n", __LINE__);
        else if ( numBytesSent != numBytesRcvd )
            printf("%d: send unexpected number of bytes", __LINE__);
        
        // See if there is more data to receive.
        numBytesRcvd = recv(clntSocket, buffer, RC_MAX_BUFSIZE, 0);
        if ( numBytesRcvd < 0 )
            DieWithSystemMessage("recv() failed.");
    }
    
CLIENT_CLOSE:    
    close(clntSocket);     // Close client socket.
    return ;
}

 


void main_listen_task(void *param)
{
    int servSock = *(int *)param;

    for (;;) 
    {               
        struct sockaddr_in clntAddr;        // Client address
        socklen_t clntAddrLen = sizeof(clntAddr);
        
        // Wait for a client to connect
        int clntSock = accept(servSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if ( clntSock < 0 )
            DieWithSystemMessage("accept() failed");
        
        // clntSock is connected to a client!
        char clntName[INET_ADDRSTRLEN];        // String to contain client address
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL )
            printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
        else
            puts("Unable to get client address");

        HandleTCPClient(clntSock);
    }

}

int show_client_info()
{
    printf("show_client_info todo \r\n");  
    return 0;  
}
#endif

int main(int argc, char *argv[]) 
{
    int ret;
    
    if(argc != 2) {
        printf("usage: %s <port> \r\n", argv[0]);
        return 0;
    }
 
    in_port_t servPort = atoi(argv[1]);
 
    // Create socket for incoming connections
    int servSock;
    if((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
        DieWithSystemMessage("Socket() failed");
    }
 
    // Construct local address structure
    struct sockaddr_in servAddr;            // Local address
    memset(&servAddr, 0, sizeof(servAddr));        // Zero out structure
    servAddr.sin_family =  AF_INET;            // IPv4 address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);    // Any incoming interface.
    servAddr.sin_port = htons(servPort);
    
    //Bind to the local address
    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 )
        DieWithSystemMessage("bind() failed");
 
    // Mark the socket so it will listen for incoming connections
    if (listen(servSock, RC_MAX_CLIENT) < 0 )
        DieWithSystemMessage("listen() failed");

    // create listen task
    pthread_t thread_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    ret = pthread_create(&thread_id, &attr, (void *)main_listen_task, &servSock);  
    if(ret != 0)  
    {  
        printf("Create pthread error!\n");  
        return -1;  
    }  

    // menu process
    for(;;)
    {
        printf("0 - quit \r\n");
        printf("1 - show client info \r\n");
        
        int menu_select;
        printf("input your choice: ");
        scanf("%d", &menu_select);

        if (menu_select == 0) break;
        if (menu_select == 1) show_client_info();
    }

}

