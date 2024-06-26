#ifndef _DEVM_MSG_H_
#define _DEVM_MSG_H_

#define DEF_LISTEN_PORT             7100

#define XMSG_T_HELLO              	0x01
#define XMSG_T_ECHO_REQ             0x02
#define XMSG_T_ECHO_ACK             0x03
#define XMSG_T_RCMD               	0x04

#define XMSG_T_USER_START         	0x10
#define XMSG_T_OAM_NULL				0x30
#define XMSG_T_MAX                	0x30

#define APP_NAME_LEN                32
#define MSG_HEAD_LEN                96	//sizeof(DEVM_MSG_S)
#define MSG_MAX_PAYLOAD             2048
#define MSG_MAGIC_NUM               0x01015AA5

typedef struct {
    int  magic_num;
    int  msg_type;
	int  serial_num;
    int  resv[4];
    char src_app[APP_NAME_LEN];
    char dst_app[APP_NAME_LEN];

	//below for user&app
    int  payload_len;
    char msg_payload[0];
}DEVM_MSG_S;

typedef int (*msg_func)(DEVM_MSG_S *rx_msg);

int devm_set_msg_func(int msg_type, msg_func func);

int devm_msg_init(int is_master);

int app_send_msg(char *dst_app, int msg_type, char *usr_data, int data_len);



#endif

