

#ifndef _DEV_MSG_H_
#define _DEV_MSG_H_

#define FALSE                   0
#define TRUE                    1

//todo: change to configurable
#define BUFFER_SIZE             1024
#define MAX_CONNECT             128

#ifndef XDEBUG_OFF
#define xprintf(...)     printf(__VA_ARGS__)
#else
#define xprintf(...)
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#define DEV_MAGIC_NUM           0x5AA5F0F0
#define DEV_NAME_MAX            32

#define DEV_CMD_REGISTER        0x0001
#define DEV_CMD_HELLO           0x0002
#define DEV_CMD_ECHO            0x0003

typedef struct 
{
    uint32  magic_num;
    uint32  app_id; //source app_id
    uint32  cmd_id;
    uint32  need_ack;
    int     ack_ret;
    int     param;
    char    payload[0];
}DEV_MSG_T;

#define DEV_STATE_NULL          0x0
#define DEV_STATE_OFFLINE       0x1
#define DEV_STATE_ONLINE        0x2

#define DEV_TYPE_SELF           0x1
#define DEV_TYPE_SERVER         0x2
#define DEV_TYPE_CLIENT         0x3

#define MAX_DEV_NUM             128
typedef struct 
{
    uint32  app_id;
    char    app_name[DEV_NAME_MAX];
    uint32  dev_state;
    uint32  dev_type;
    int     socket_id;
    pthread_t thread_id;
}DEV_INFO_T;

typedef int (* MSG_PROC)(DEV_INFO_T *dev_info, char *msg_buf, int buf_len);

#define DEV_FLAGS_UDS           0x01    //default is INET
#define DEV_FLAGS_NEED_REG      0x02

int dev_msg_init(int app_id, int port);
int dev_msg_send(int app_id, uint32 cmd_id, uint32 need_ack, char *msg_buf, int msg_len);

int dev_cmd_register(uint32 cmd_id, MSG_PROC func);
int dev_cmd_list(int argc, char **argv);
int dev_cmd_connect(int argc, char **argv);
int dev_cmd_echo(int argc, char **argv);

#endif

