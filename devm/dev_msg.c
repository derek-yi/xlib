#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dev_msg.h"

#if 1

typedef struct 
{
    uint32  app_id;
    char    app_name[DEV_NAME_MAX];
    uint32  dev_state;
    uint32  dev_type;
    int     socket_id;
    pthread_t thread_id;
}DEV_INFO_T;

DEV_INFO_T dev_list[MAX_DEV_NUM] = {0};

#define APP_NAME_FMT    "app%04d"

typedef struct 
{
    int     listen_fd;
    int     flags;
}TASK_PARAM_T;

typedef struct 
{
    uint32      cmd_id;
    MSG_PROC    func;
}MSG_PROC_T;

int dev_msg_echo(uint32 dev_id, char *msg_buf, int buf_len)
{
    DEV_MSG_T *dev_msg = (DEV_MSG_T *)msg_buf;

    xprintf("echo cmd dev %d: %s \n", dev_msg->app_id, dev_msg->payload);
    
    if (dev_msg->need_ack) {
        dev_msg_send(dev_msg->app_id, DEV_CMD_ECHO, FALSE, "echo ACK", 16);
    }

    return 0;
}

MSG_PROC_T msg_proc_list[128] = {
    {DEV_CMD_ECHO, dev_msg_echo},
    {0, NULL},
};

int dev_msg_proc(uint32 dev_id, char *msg_buf, int buf_len)
{
    DEV_MSG_T *dev_msg = (DEV_MSG_T *)msg_buf;
    int i, ret = 0;

    if (dev_msg->magic_num != DEV_MAGIC_NUM) {
        xprintf("wrong magic 0x%x\n", dev_msg->magic_num);
        return -1;
    }
    
    xprintf("recv cmd 0x%x from dev %d \n", dev_msg->cmd_id, dev_msg->app_id);
    for (i = 0; i < sizeof(msg_proc_list)/sizeof(MSG_PROC_T); i++) {
        if ( (msg_proc_list[i].cmd_id == dev_msg->cmd_id) 
            && (msg_proc_list[i].func != NULL) ) {
            ret = msg_proc_list[i].func(dev_id, msg_buf, buf_len);
            break;
        }
    }
    
    return ret;
}

void client_rx_task(void *param)
{
    DEV_INFO_T *dev_info = (DEV_INFO_T *)param;
    char buffer[BUFFER_SIZE];
    int socket_id, rx_len;

    socket_id = dev_info->socket_id;
    while (dev_info->dev_state) {
        rx_len = recv(socket_id, buffer, BUFFER_SIZE, 0);
        if (rx_len < sizeof(DEV_MSG_T)) {
            xprintf("broken msg, len %d\n", rx_len);
            break;
        }
        
        dev_msg_proc(dev_info->app_id, buffer, rx_len);
    }

    close(socket_id);
    dev_info->dev_state = DEV_STATE_NULL;
}

int create_rx_task(uint32 dev_type, int app_id, int socket_id)
{
    int i, free_idx, ret;
    pthread_t thread_id;

    for (i = 0, free_idx = -1; i < MAX_DEV_NUM; i++) {
        if (!dev_list[i].dev_state && free_idx < 0) {
            free_idx = i;
        } else if (dev_list[i].dev_state && dev_list[i].app_id == app_id) {
            xprintf("exist app_id %d\n", app_id);
            return -1;
        }
    }

    if (free_idx < 0) {
        xprintf("full dev_list\n");
        return -1;
    }

    dev_list[free_idx].dev_state = DEV_STATE_ONLINE;
    dev_list[free_idx].dev_type = dev_type;
    dev_list[free_idx].app_id = app_id;
    snprintf(dev_list[free_idx].app_name, DEV_NAME_MAX, APP_NAME_FMT, app_id);
    dev_list[free_idx].socket_id = socket_id;
    
    ret = pthread_create(&thread_id, NULL, (void *)client_rx_task, &dev_list[free_idx]);  
    if (ret != 0)  {
        printf("pthread_create failed!\n");  
        dev_list[free_idx].dev_state = DEV_STATE_NULL;
        return -1;  
    }  
    dev_list[free_idx].thread_id = thread_id;
    
    return 0;
}

void dev_listen_task(void *param)
{
    int new_fd, len, ret;
    struct sockaddr_un un_addr;
    struct sockaddr_in inet_addr;
    TASK_PARAM_T *task_param = (TASK_PARAM_T *)param;

    while(1) {
        char buffer[BUFFER_SIZE];
        socklen_t addr_len;
        DEV_MSG_T *dev_msg;
        
        if (task_param->flags & DEV_FLAGS_UDS) {
            addr_len = sizeof(un_addr);
            new_fd = accept(task_param->listen_fd, (struct sockaddr*)&un_addr, &addr_len);
        } else {
            addr_len = sizeof(inet_addr);
            new_fd = accept(task_param->listen_fd, (struct sockaddr*)&inet_addr, &addr_len);
        }

        if (new_fd < 0) {
            xprintf("accept failed\n");
            //rebuild_listen_socket();//todo
            continue;
        }

        bzero(buffer, BUFFER_SIZE);
        len = recv(new_fd, buffer, BUFFER_SIZE, 0);
        if (len < sizeof(DEV_MSG_T)){
            xprintf("recv failed, len %d\n", len);
            close(new_fd);
            continue;
        } 

        //first msg should be register cmd
        dev_msg = (DEV_MSG_T *)buffer;
        if (dev_msg->magic_num != DEV_MAGIC_NUM) {
            xprintf("wrong magic num 0x%x\n", dev_msg->magic_num);
            close(new_fd);
            continue;
        }
        if ( (task_param->flags&DEV_FLAGS_NEED_REG) && (dev_msg->cmd_id != DEV_CMD_REGISTER)) {
            xprintf("need reg cmd 0x%x\n", dev_msg->cmd_id);
            close(new_fd);
            continue;
        }

        ret = create_rx_task(DEV_TYPE_CLIENT, dev_msg->app_id, new_fd);
        if (ret < 0){
            xprintf("client create failed\n");
            close(new_fd);
            continue;
        } 
    }
    
    xprintf("listen task abort\n");
    close(task_param->listen_fd);
    free(task_param);
}

int dev_uds_init(int app_id, int flags)
{
    int listen_fd, ret;
    pthread_t thread_id;
    struct sockaddr_un un_addr;
    TASK_PARAM_T *task_param;
    char app_name[DEV_NAME_MAX];

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0){
        xprintf("request socket failed!\n");
        return -1;
    }

    sprintf(app_name, APP_NAME_FMT, app_id);
    un_addr.sun_family = AF_UNIX;
    unlink(app_name);
    strcpy(un_addr.sun_path, app_name);
    if (bind(listen_fd,(struct sockaddr *)&un_addr, sizeof(un_addr)) < 0 ){
        xprintf("bind failed!\n");
        return -1;
    }
    
    if ( listen(listen_fd, MAX_CONNECT ) < 0){
        xprintf("listen failed!\n");
        return -1;
    }

    task_param = (TASK_PARAM_T *)malloc(sizeof(TASK_PARAM_T));
    if ( task_param == NULL){
        xprintf("malloc failed!\n");
        close(listen_fd);
        return -1;
    }

    task_param->listen_fd = listen_fd;
    task_param->flags = flags;
    ret = pthread_create(&thread_id, NULL, (void *)dev_listen_task, (void *)task_param);  
    if (ret != 0)  {
        printf("pthread_create failed!\n");  
        return -1;  
    }  

    dev_list[0].dev_state = DEV_STATE_ONLINE;
    dev_list[0].dev_type = DEV_TYPE_SELF;
    dev_list[0].app_id = app_id;
    snprintf(dev_list[0].app_name, DEV_NAME_MAX, APP_NAME_FMT, app_id);
    dev_list[0].socket_id = listen_fd;
    dev_list[0].thread_id = thread_id;

    return 0;
}

int dev_inet_init(int listen_port, int flags)
{
    int listen_fd, ret;
    pthread_t thread_id;
    struct sockaddr_in inet_addr;
    TASK_PARAM_T *task_param;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        xprintf("Request socket failed!\n");
        return -1;
    }
    
    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family =  AF_INET; 
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(listen_port);
    if (bind(listen_fd,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        xprintf("bind failed!\n");
        return -1;
    }
    
    if ( listen(listen_fd, MAX_CONNECT ) < 0){
        xprintf("listen failed!\n");
        return -1;
    }

    task_param = (TASK_PARAM_T *)malloc(sizeof(TASK_PARAM_T));
    if ( task_param == NULL){
        xprintf("malloc failed!\n");
        close(listen_fd);
        return -1;
    }

    task_param->listen_fd = listen_fd;
    task_param->flags = flags;
    ret = pthread_create(&thread_id, NULL, (void *)dev_listen_task, (void *)task_param);  
    if (ret != 0)  {
        printf("pthread_create failed!\n");  
        return -1;  
    }  

    dev_list[1].dev_state = DEV_STATE_ONLINE;
    dev_list[1].dev_type = DEV_TYPE_SELF;
    snprintf(dev_list[1].app_name, DEV_NAME_MAX, "%s", "inet");
    dev_list[1].socket_id = listen_fd;
    dev_list[1].thread_id = thread_id;

    return 0;
}

uint32 dev_local_appid()
{
    return dev_list[0].app_id;
}

#endif

#if 1

int dev_msg_init(int app_id, int listen_port)
{
    int ret;

    ret = dev_uds_init(app_id, DEV_FLAGS_UDS | DEV_FLAGS_NEED_REG);
    if (ret < 0){
        xprintf("uds create failed\n");
        return -1;
    } 

    if (listen_port > 0) {
        ret = dev_inet_init(listen_port, DEV_FLAGS_INET | DEV_FLAGS_NEED_REG);
        if (ret < 0){
            xprintf("uds create failed\n");
            return -1;
        } 
    }

    return 0;
}

int dev_msg_send(int app_id, uint32 cmd_id, uint32 need_ack, char *msg_buf, int msg_len)
{
    int i, ret;
    char buffer[BUFFER_SIZE];
    DEV_MSG_T *dev_msg;
    
    for(i = 0; i < MAX_DEV_NUM; i++) {
        if (dev_list[i].dev_state && dev_list[i].app_id == app_id) {
            break;
        }
    }
    if (i == MAX_DEV_NUM ) {
        printf("invalid app_id\n");
        return 0;
    }

    dev_msg = (DEV_MSG_T *)buffer;
    dev_msg->magic_num = DEV_MAGIC_NUM;
    dev_msg->cmd_id = cmd_id;
    dev_msg->app_id = dev_local_appid();
    dev_msg->need_ack = need_ack; 
    memcpy(buffer + sizeof(DEV_MSG_T), msg_buf, msg_len);

    if (dev_list[i].dev_type == DEV_TYPE_SELF) {
        ret = dev_msg_proc(dev_local_appid(), buffer, sizeof(DEV_MSG_T) + msg_len);
        return ret;
    }
    
    ret = send(dev_list[i].socket_id, buffer, sizeof(DEV_MSG_T) + msg_len, 0);
    if (ret <= 0) {
        printf("send failed!\n");
        return -1;
    }

    return 0;
}


int dev_cmd_register(uint32 cmd_id, MSG_PROC func)
{
    int i;
    int free_idx = -1;
    
    for (i = 0; i < sizeof(msg_proc_list)/sizeof(MSG_PROC_T); i++) {
        if (msg_proc_list[i].cmd_id == 0) {
            if (free_idx < 0) free_idx = i;
        } else if (msg_proc_list[i].cmd_id == cmd_id) {
            return -1;
        }
    }
    
    if (free_idx < 0) return -1;//full
    msg_proc_list[free_idx].cmd_id = cmd_id;
    msg_proc_list[free_idx].func = func;
    
    return 0;
}

int dev_cmd_list(int argc, char **argv)
{
    int i;

    for(i = 0; i < MAX_DEV_NUM; i++) {
        if(dev_list[i].dev_state) {
            printf("%d: type=%d state=%d appid=%d(%s)\n", i, 
                    dev_list[i].dev_type, dev_list[i].dev_state, 
                    dev_list[i].app_id, dev_list[i].app_name);
        }
    }
    return 0;
}

int dev_cmd_connect(int argc, char **argv)
{
    struct sockaddr_un un;
    struct sockaddr_in servAddr;
    int sock_fd = 0;
    int ret, app_id;
    DEV_MSG_T dev_msg;

    if (argc < 3) {
        printf("Usage: \n");
        printf("  %s uds <app-id>\n", argv[0]);
        printf("  %s inet <app-id> <ip> <port>\n", argv[0]);
        return 0;
    }

    app_id = atoi(argv[2]);
    if (!memcmp(argv[1], "uds", 3)) {
        un.sun_family = AF_UNIX;
        //strcpy(un.sun_path, argv[2]);
         sprintf(un.sun_path, APP_NAME_FMT, app_id);
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd < 0){
            printf("request socket failed\n");
            return -1;
        }
        
        if (connect(sock_fd, (struct sockaddr *)&un, sizeof(un)) < 0) {
            printf("connect socket failed\n");
            close(sock_fd);
            return -1;
        }
    } else if (!memcmp(argv[1], "inet", 4)) {
        if (argc < 5) {
            printf("usage: %s inet <app-name> <ip> <port>\n", argv[0]);
            return 0;
        }

        memset(&servAddr, 0, sizeof(servAddr) );
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(atoi(argv[4]));
        inet_pton(AF_INET, argv[3], &servAddr.sin_addr.s_addr);
        sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if(sock_fd < 0){
            printf("request socket failed\n");
            return -1;
        }
        
        if(connect(sock_fd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
            printf("connect socket failed\n");
            close(sock_fd);
            return -1;
        }
    }

    dev_msg.magic_num = DEV_MAGIC_NUM;
    dev_msg.cmd_id = DEV_CMD_REGISTER;
    dev_msg.app_id = dev_local_appid();
    ret = send(sock_fd, &dev_msg, sizeof(DEV_MSG_T), 0);
    if (ret <= 0) {
        printf("send failed!\n");
        close(sock_fd);
        return -1;
    }
    
    ret = create_rx_task(DEV_TYPE_SERVER, app_id, sock_fd);
    if (ret < 0)  {
        printf("create_rx_task failed!\n");  
        close(sock_fd);
        return -1;
    }  

    return 0;
}

int dev_cmd_echo(int argc, char **argv)
{
    int ret, app_id;
    
    if (argc < 3) {
        printf("Usage: \n");
        printf("  %s <app-id> <s1> ...\n", argv[0]);
        return 0;
    }

    app_id = atoi(argv[1]);
    ret = dev_msg_send(app_id, DEV_CMD_ECHO, TRUE, "say hello", 16);
    if (ret < 0)  {
        printf("dev_msg_send failed!\n");  
        return -1;
    }  

    return 0;
}

#endif

