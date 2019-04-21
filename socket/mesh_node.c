
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "mesh_node.h"

#define MNODE_MAX_NUM           16
#define MNODE_BUFSIZE           2048

#define MNODE_STATE_NULL        0
#define MNODE_STATE_INIT        1
#define MNODE_STATE_ONLINE      2

typedef struct _mesh_node
{
    int     socket_id;
    int     ip_addr;
    char    ip_string[32];
    int     tcp_port;

    uint32  role;  // 0-server, 1-client
    uint32  state;
    uint32  age_credit;
    uint32  rx_pkt;
    uint32  tx_pkt;
}mesh_node;

typedef struct _queue_msg
{
    uint32  mnode_id;
    char    *mn_msg;
} queue_msg;


static mesh_node mnode_list[MNODE_MAX_NUM];



#if T_DESC("MNODE_CORE", 1)

static int listen_socket = 0;

int user_rx_qid = -1;
int user_tx_qid = -1;

int mnode_msg_enqueue(int index, mnode_msg *mn_msg)
{
    queue_msg rx_msg;
    int msg_len;

    msg_len = sizeof(mnode_msg) + mn_msg->data_len;
    rx_msg.mn_msg = (char *)malloc(msg_len);
    if ( rx_msg.mn_msg == NULL ){  
        printf("%d: malloc() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    rx_msg.mnode_id = index;
    memcpy(rx_msg.mn_msg, mn_msg, msg_len);

    int ret = msgsnd(user_rx_qid, (void *)&rx_msg, sizeof(queue_msg), 0);
    if ( ret < 0 ){  
        printf("%d: msgsnd() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  
    
    return VOS_OK;  
}

int mnode_send_msg(int index, mnode_msg *mn_msg)
{
    queue_msg rx_msg;
    int msg_len;

    msg_len = sizeof(mnode_msg) + mn_msg->data_len;
    rx_msg.mn_msg = (char *)malloc(msg_len);
    if ( rx_msg.mn_msg == NULL ){  
        printf("%d: malloc() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    rx_msg.mnode_id = index;
    memcpy(rx_msg.mn_msg, mn_msg, msg_len);

    int ret = msgsnd(user_tx_qid, (void *)&rx_msg, sizeof(queue_msg), 0);
    if ( ret < 0 ){  
        printf("%d: msgsnd() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  
    
    return VOS_OK;  
}

int mnode_cmd_hello(int index, mnode_msg *mn_msg)
{
    mnode_list[index].state = MNODE_STATE_ONLINE;
    mnode_list[index].age_credit = 9999;

    // send ack
    mn_msg->magic_num = MNODE_MAGIC_NUM - 1; // no loopback
    mn_msg->ack = VOS_OK;
    int tx_len = send(mnode_list[index].socket_id, 
                        (void *)mn_msg, sizeof(mnode_msg), 0);
    if ( tx_len != sizeof(mnode_msg) ){  
        printf("%d: send() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  
    mnode_list[index].tx_pkt++;
    
    return VOS_OK; 
}

int mnode_cmd_echo(int index, mnode_msg *mn_msg)
{
    // show string
    printf("%d: recv echo cmd \r\n", __LINE__);
    printf("string: %s \r\n", mn_msg->data);
    
    // send ack
    mn_msg->magic_num = MNODE_MAGIC_NUM - 1; // no loopback
    mn_msg->ack = VOS_OK;
    int tx_len = send(mnode_list[index].socket_id, 
                        (void *)mn_msg, sizeof(mnode_msg) + mn_msg->data_len, 0);
    if ( tx_len != sizeof(mnode_msg) + mn_msg->data_len ){  
        printf("%d: send() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  
    mnode_list[index].tx_pkt++;
    
    return VOS_OK; 
}

int mnode_pkt_proc(int index, char *packet, int pkt_len)
{
    int ret;
    mnode_msg *mn_msg = (mnode_msg *)packet;

    if (packet == NULL || pkt_len < sizeof(mnode_msg) ) {  
        printf("%d: illegal packet \r\n", __LINE__);
        return VOS_ERR;  
    }  

    if ( mn_msg->magic_num != MNODE_MAGIC_NUM ) {  
        printf("%d: illegal magic_num 0x%x \r\n", __LINE__, mn_msg->magic_num);
        return VOS_ERR;  
    }  

    if (mn_msg->cmd == MNODE_CMD_HELLO) {
        ret = mnode_cmd_hello(index, mn_msg);
    }
    else if (mn_msg->cmd == MNODE_CMD_ECHO) {
        ret = mnode_cmd_echo(index, mn_msg);
    }
    else { // user cmd
        if (pkt_len < sizeof(mnode_msg) + mn_msg->data_len) {  
            printf("%d: illegal packet \r\n", __LINE__);
            return VOS_ERR;  
        }  
        ret = mnode_msg_enqueue(index, mn_msg);
    }

    return ret;
}

void *mnode_socket_rx_task(void *param)
{
    int index = (int)param;
    int socket_id = mnode_list[index].socket_id;
    char buffer[MNODE_BUFSIZE];  

    while ( mnode_list[index ].state > MNODE_STATE_NULL )
    {
        // Receive message from client.
        ssize_t numBytesRcvd = recv(socket_id, buffer, MNODE_BUFSIZE, 0);
        if ( numBytesRcvd <= 0 ) {  
            printf("%d: recv() failed \r\n", __LINE__);
            break;  
        }  

        // packet process
        mnode_list[index].rx_pkt++;
        mnode_pkt_proc(index, buffer, numBytesRcvd);
    }

    printf("%d: mnode exit, ip %s port %d \r\n", __LINE__,
            mnode_list[index ].ip_string, mnode_list[index ].tcp_port);
    close(socket_id);
    mnode_list[index ].socket_id = 0;
}
 
void mnode_show_list(void)
{
    for(int i = 0; i < MNODE_MAX_NUM; i++) {
        if (mnode_list[i].state > MNODE_STATE_NULL ) {
            printf("%d: state %d, ip_addr %s(%d), ", i,
            mnode_list[i].state, mnode_list[i].ip_string, mnode_list[i].tcp_port);
            printf("age %d, tx_pkt %d, rx_pkt %d \r\n",
            mnode_list[i].age_credit, mnode_list[i].tx_pkt, mnode_list[i].rx_pkt);
        }
    }
}
 
int mnode_add_node(int socket_id, int peer_ip, int tcp_port)
{
    int index;
    char clntName[INET_ADDRSTRLEN];  

    // duplicate check
    for(index = 0; index < MNODE_MAX_NUM; index++) {
        if (mnode_list[index].state != MNODE_STATE_NULL ) {
            if ( (mnode_list[index].ip_addr == peer_ip) 
                && (mnode_list[index].tcp_port == tcp_port) )  {  
                printf("%d: node exist \r\n", __LINE__);
                return VOS_ERR;  
            }  
        }
    }

    // save to mnode_list
    for(index = 0; index < MNODE_MAX_NUM; index++) {
        if (mnode_list[index].state == MNODE_STATE_NULL ) {
            mnode_list[index].state = MNODE_STATE_INIT;
            mnode_list[index].socket_id = socket_id;
            mnode_list[index].ip_addr = peer_ip;
            inet_ntop(AF_INET, &peer_ip, clntName, sizeof(clntName));
            sprintf(mnode_list[index].ip_string, "%s", clntName);
            mnode_list[index].tcp_port = tcp_port;

            mnode_list[index].age_credit = 300;
            mnode_list[index].rx_pkt = 0;
            mnode_list[index].tx_pkt = 0;
            break;
        }
    }

    // no space
    if (index == MNODE_MAX_NUM) {
        return VOS_ERR;
    }

    // create rx task
    pthread_attr_t attr;
    pthread_t thread_id;
    pthread_attr_init(&attr);
    int ret = pthread_create(&thread_id, &attr, (void *)mnode_socket_rx_task, (void *)index);  
    if(ret != 0)  {  
        printf("%d: pthread_create() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    return VOS_OK;
}

void *mnode_listen_task(void *param)
{
    for (;;) {  // Run forever
        struct sockaddr_in clntAddr; 
        socklen_t clntAddrLen = sizeof(clntAddr);
        
        // Wait for a client to connect
        int clntSock = accept(listen_socket, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if ( clntSock < 0 ) {
            printf("%d: socket() failed \r\n", __LINE__);
            continue;
        }

        int ret = mnode_add_node(clntSock, clntAddr.sin_addr.s_addr, clntAddr.sin_port);
        if ( ret != VOS_OK ) {
            printf("%d: mnode_add_node() failed \r\n", __LINE__);
            close(clntSock);
            continue;
        }
    }
}

void *mnode_aging_task(void *param)
{
    for(;;)
    {
        sleep(10);
        for(int i = 0; i < MNODE_MAX_NUM; i++) {
            if (mnode_list[i].state > MNODE_STATE_NULL ) {
                if (mnode_list[i].age_credit > 0) mnode_list[i].age_credit--;
                
                if (mnode_list[i].age_credit == 0) {
                    mnode_list[i].state = MNODE_STATE_NULL;
                }
            }
        }
    }
}

void *mnode_socket_tx_task(void *param)
{
    int ret;
    queue_msg tx_msg;
    mnode_msg *mn_msg;
    
    user_tx_qid = msgget(IPC_PRIVATE, 0666);
    if(user_tx_qid == -1) {
        printf("msgget error\n");
        return NULL;
    }

    for(;;)
    {
        ret = msgrcv(user_tx_qid, (void *)&tx_msg, sizeof(queue_msg), 0, 0);
        if (ret < 0) {
            printf("msgrcv() error\n");
            continue ;
        }

        mn_msg = (mnode_msg *)tx_msg.mn_msg;
        printf("%d: send user cmd \r\n", __LINE__);
        printf("magic 0x%x, cmd %d \r\n", mn_msg->magic_num, mn_msg->cmd);

        int index = tx_msg.mnode_id;
        int tx_len = send(mnode_list[index].socket_id, 
                            (void *)mn_msg, sizeof(mnode_msg) + mn_msg->data_len, 0);
        if ( tx_len != sizeof(mnode_msg) ){  
            printf("%d: send() failed \r\n", __LINE__);
            continue ;
        }  
        mnode_list[index].tx_pkt++;
    }
}

void *mnode_user_cmd_task(void *param)
{
    int ret;
    queue_msg rx_msg;
    mnode_msg *mn_msg;
    
    user_rx_qid = msgget(IPC_PRIVATE, 0666);
    if(user_rx_qid == -1) {
        printf("msgget error\n");
        return NULL;
    }

    for(;;)
    {
        ret = msgrcv(user_rx_qid, (void *)&rx_msg, sizeof(queue_msg), 0, 0);
        if (ret < 0) {
            printf("msgrcv error\n");
            continue ;
        }

        mn_msg = (mnode_msg *)rx_msg.mn_msg;
        printf("%d: recv user cmd \r\n", __LINE__);
        printf("magic 0x%x, cmd %d \r\n", mn_msg->magic_num, mn_msg->cmd);

        // todo
    }
}

int mnode_server_init(int port)
{
    int ret;
    
    // Create socket for incoming connections
    if((listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
        printf("%d: socket() failed \r\n", __LINE__);
        return VOS_ERR;
    }

    // set SO_REUSEADDR
    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    // Construct local address structure
    struct sockaddr_in servAddr;            // Local address
    memset(&servAddr, 0, sizeof(servAddr));        // Zero out structure
    servAddr.sin_family =  AF_INET;            // IPv4 address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);    // Any incoming interface.
    servAddr.sin_port = htons(port);
    
    // Bind to the local address
    if (bind(listen_socket, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 ) {
        printf("%d: bind() failed \r\n", __LINE__);
        return VOS_ERR;
    }
 
    // Mark the socket so it will listen for incoming connections
    if (listen(listen_socket, MNODE_MAX_NUM) < 0 ){
        printf("%d: listen() failed \r\n", __LINE__);
        return VOS_ERR;
    }

    // create listen task
    pthread_attr_t attr;
    pthread_t thread_id;
    pthread_attr_init(&attr);
    ret = pthread_create(&thread_id, &attr, (void *)mnode_listen_task, NULL);  
    if(ret != 0)  {  
        printf("%d: pthread_create() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    ret = pthread_create(&thread_id, &attr, (void *)mnode_aging_task, NULL);  
    if(ret != 0)  {  
        printf("%d: pthread_create() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    ret = pthread_create(&thread_id, &attr, (void *)mnode_user_cmd_task, NULL);  
    if(ret != 0)  {  
        printf("%d: pthread_create() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    ret = pthread_create(&thread_id, &attr, (void *)mnode_socket_tx_task, NULL);  
    if(ret != 0)  {  
        printf("%d: pthread_create() failed \r\n", __LINE__);
        return VOS_ERR;  
    }  

    return VOS_OK;
}

int mnode_connect_node(char *ip_addr, int port)
{
    int client_socket = -1;
    
    // Create server address
    struct sockaddr_in servAddr;
    memset( &servAddr, 0, sizeof(servAddr) );
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(port);

    // Create local socket
    if ( ( client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 ) {
        printf("%d: socket() failed \r\n", __LINE__);
        return VOS_ERR;
    } 

    // Connect to server
    if ( (connect( client_socket, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0  ) {
        printf("%d: connect() failed \r\n", __LINE__);
        return VOS_ERR;
    }

    int ret = mnode_add_node(client_socket, servAddr.sin_addr.s_addr, port);
    if ( ret != VOS_OK ) {
        printf("%d: mnode_add_node() failed \r\n", __LINE__);
        close(client_socket);
        return VOS_ERR;
    }

    return VOS_OK;  
}

int mnode_send_echo(int node_id, char *echo_str)
{   
    int ret;
    char buffer[128];
    mnode_msg *mn_msg = (mnode_msg *)buffer;

    mn_msg->magic_num = MNODE_MAGIC_NUM;
    mn_msg->cmd = MNODE_CMD_ECHO;
    mn_msg->data_len = strlen(echo_str) + 1;
    memcpy(mn_msg->data, echo_str, mn_msg->data_len);

    return mnode_send_msg(node_id, mn_msg);
}

int mnode_send_user_cmd(int node_id, char *cmd_str)
{   
    int ret;
    char buffer[128];
    mnode_msg *mn_msg = (mnode_msg *)buffer;

    mn_msg->magic_num = MNODE_MAGIC_NUM;
    mn_msg->cmd = MNODE_CMD_USER_CMD;
    mn_msg->data_len = strlen(cmd_str) + 1;
    memcpy(mn_msg->data, cmd_str, mn_msg->data_len);

    return mnode_send_msg(node_id, mn_msg);
}

int mnode_send_hello(int node_id)
{   
    int ret;
    char buffer[128];
    mnode_msg *mn_msg = (mnode_msg *)buffer;

    mn_msg->magic_num = MNODE_MAGIC_NUM;
    mn_msg->cmd = MNODE_CMD_HELLO;
    mn_msg->data_len = 0;

    return mnode_send_msg(node_id, mn_msg);
}


#endif

#ifndef BUILD_MNODE_LIB

int my_getline(char* line, int max_size)
{
    int c;
    int len = 0;

    //fflush(stdin);
    while( (c=getchar()) != EOF && c != EOF);
    
    while( (c=getchar()) != EOF && len < max_size){
        line[len++]=c;
        if ('\n' == c)
        break;
    }

    line[len] ='\0';
    return len;
}

int main(int argc, char *argv[]) 
{
    char buffer[64];
    int node_id; 
    
    if(argc != 2) {
        printf("Usage: %s <port> \r\n", argv[0]);
        return VOS_ERR; 
    }

    int serv_port = atoi(argv[1]);
 
    // Create socket and listen task
    mnode_server_init(serv_port);

    for(;;) {
        printf("--------------------------------------------------- \r\n");
        printf("0 - quit \r\n");
        printf("1 - connect node \r\n");
        printf("2 - show node list \r\n");
        printf("3 - say hello \r\n");
        printf("4 - send echo \r\n");
        printf("5 - send user cmd \r\n");
        printf("--------------------------------------------------- \r\n");
        
        int menu_select;
        printf("input your choice: ");
        scanf("%d", &menu_select);

        if (menu_select == 0) break;
        
        if (menu_select == 1) {
            char ip_addr[64];
            int port; 
            
            printf("\r\ninput ip address: ");
            scanf("%s", ip_addr);
            printf("\r\ninput tcp port: ");
            scanf("%s", buffer);
            port = atoi(buffer);

            mnode_connect_node(ip_addr, port);
        }
        
        if (menu_select == 2) { 
            mnode_show_list();
        }
        
        if (menu_select == 3) {
            printf("\r\ninput node id: ");
            scanf("%s", buffer);
            node_id = atoi(buffer);
            
            mnode_send_hello(node_id);
        }
        
        if (menu_select == 4) {
            printf("\r\ninput node id: ");
            scanf("%s", buffer);
            node_id = atoi(buffer);

            printf("\r\ninput echo string: ");
            scanf("%s", buffer);

            mnode_send_echo(node_id, buffer);
        }
        
        if (menu_select == 5) {
            printf("\r\ninput node id: ");
            scanf("%s", buffer);
            node_id = atoi(buffer);

            printf("\r\ninput cmd string: ");
            scanf("%s", buffer);

            mnode_send_user_cmd(node_id, buffer);
        }
    }
    return VOS_OK;
}
#endif 

