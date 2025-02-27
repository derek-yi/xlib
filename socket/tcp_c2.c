#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TX_BLK_SZ   2000

int tx_total = 0;

void DieWithSystemMessage(const char *msg) 
{
    perror(msg);
    exit(1);
}


int main(int argc, char **argv){
 
    if ( argc < 4) {
        DieWithSystemMessage("cmd <IP> <PORT> <len>");
    }
 
    int servPort = atoi(argv[2]);
    int sock, len;
    char msg[4096];
    int i, j;

    if ( ( sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
        DieWithSystemMessage("socket() failed.");
    } 
 
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
    len = atoi(argv[3]);

    for (i = 0; i < len/TX_BLK_SZ; i++) {
        for (j = 0; j < TX_BLK_SZ; j++) msg[j] = i;
        int sentByteLen = send( sock, msg, TX_BLK_SZ, 0);
        tx_total += sentByteLen;
        if ( sentByteLen > 0 ) {
            printf("send: %d bytes, total %d\n", sentByteLen, tx_total);
        } else {
            printf("send failed(%d): %d(%s)\n", sentByteLen, errno, strerror(errno));
            break;
        }
    }

    if (len%TX_BLK_SZ) {
        for (j = 0; j < len%TX_BLK_SZ; j++) msg[j] = i;
        int sentByteLen = send( sock, msg, j, 0);
        tx_total += sentByteLen;
        if ( sentByteLen > 0 ) {
            printf("send: %d bytes, total %d\n", sentByteLen, tx_total);
        } else {
            printf("send failed(%d): %d(%s)\n", sentByteLen, errno, strerror(errno));
        }
    }

    getchar(); //pause
    close(sock);
    
    printf("end!\n");
}



