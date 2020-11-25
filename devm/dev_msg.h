

#ifndef _DEV_MSG_H_
#define _DEV_MSG_H_

#define FALSE                   0
#define TRUE                    1

//todo: change to configurable
#define BUFFER_SIZE             1024
#define MAX_CONNECT             128

#define _DEBUG

#ifdef _DEBUG
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
    uint32  app_id; //src app_id
    uint32  cmd_id;
    uint32  need_ack;
    int     ack_ret;
    int     param[4];
    int     payload_len;
    char    payload[0];
}DEV_MSG_T;

#define DEV_STATE_NULL          0x0
#define DEV_STATE_OFFLINE       0x1
#define DEV_STATE_ONLINE        0x2

#define DEV_TYPE_SERVER         0x1
#define DEV_TYPE_CLIENT         0x2

#define MAX_DEV_NUM             128


typedef int (* MSG_PROC)(uint32 dev_id, char *msg_buf, int buf_len);

#define DEV_FLAGS_UDS           0x01
#define DEV_FLAGS_INET          0x02
#define DEV_FLAGS_NEED_REG      0x04
#define DEV_FLAGS_CHECK_APPID   0x08

int dev_msg_init(int app_id, int port);

int dev_msg_send(int dev_id, DEV_MSG_T *tx_msg, DEV_MSG_T *rx_msg);
int app_msg_send(int dev_id, uint32 cmd_id, uint32 need_ack, char *tx_buf, int tx_len);
int app_msg_send2(int dev_id, uint32 cmd_id, uint32 need_ack, char *tx_buf, int tx_len, char *rx_buf, int rx_len);

int dev_cmd_register(uint32 cmd_id, MSG_PROC func);

int cli_dev_list(int argc, char **argv);
int cli_dev_connect(int argc, char **argv);
int cli_send_echo(int argc, char **argv);

#endif

