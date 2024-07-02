#include "xmodule.h"

#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_CONNECT_NUM 32

msg_func msg_func_list[XMSG_T_MAX] = {0};

typedef struct {
    char app_name[APP_NAME_LEN];
    int  sock_id;
    int  used;
    unsigned long  rx_cnt;
    unsigned long  tx_cnt;
}SOCK_INFO;

SOCK_INFO sock_list[MAX_CONNECT_NUM] = {0};

pthread_mutex_t tx_mutex = PTHREAD_MUTEX_INITIALIZER;

struct timeval glb_timeout = {60, 0};

int local_is_master = 0;

int local_sock = -1;

static int xmsg_qid = -1;

#define FORCE_CLOSE(s)	{ if (s > 0) {shutdown(s, SHUT_RDWR);  close(s); } s = -1; }

#if 1

int devm_delete_sock(int sock_id)
{
	xlog_info("delete sock %d", sock_id);

    pthread_mutex_lock(&tx_mutex);
	for (int i = 0; i < MAX_CONNECT_NUM; i++) {
		if (sock_list[i].used == FALSE) {
			continue;
		} else if (sock_list[i].sock_id == sock_id) {
		    memset(&sock_list[i], 0, sizeof(SOCK_INFO));
            sock_list[i].sock_id = -1;
			break;
		}
	}
    
	pthread_mutex_unlock(&tx_mutex);
	return VOS_OK;
}

int devm_update_sock(int sockid, char *app_name)
{
	int i, j = -1;
	int match = 0;

	for (i = 0; i < MAX_CONNECT_NUM; i++) {
		if (sock_list[i].used == FALSE) {
			if (j < 0) j = i;   //get first unused id
		} else if (sock_list[i].sock_id == sockid) {
			j = i; match = 1;   //same sock, update name
			break;
		}
	}

    if (match == 0) {
        for (i = 0; i < MAX_CONNECT_NUM; i++) {
            if (sock_list[i].used == FALSE) {
                continue;
            } else if (strcmp(sock_list[i].app_name, app_name) == 0) {
                j = i; match = 2;   //same name, update sock
                break;
            }
        }
    }

	if (match == 1) {			
		if (strcmp(sock_list[j].app_name, app_name) != 0) {
            xlog_info("update sock %d, app_name %s", sockid, app_name);
			snprintf(sock_list[j].app_name, APP_NAME_LEN, "%s", app_name);
		}
	} else if (match == 2) {			
		sock_list[j].sock_id = sockid;
	} else if (j < 0) {
        xlog(XLOG_ERROR, "full");
        return VOS_ERR;
	} else {
		xlog_info("new sock %d, app_name %s", sockid, app_name);
		sock_list[j].sock_id = sockid;
		snprintf(sock_list[j].app_name, APP_NAME_LEN, "%s", app_name);
		sock_list[j].used = TRUE;
	}

	return j;
}

int devm_set_msg_func(int msg_type, msg_func func)
{
    if (msg_type >= XMSG_T_MAX || msg_type < XMSG_T_USER_START) {
        return VOS_ERR;
    }

    msg_func_list[msg_type] = func;
    return VOS_OK;
}

int devm_msg_send(char *dst_app, DEVM_MSG_S *tx_msg, int msg_len)
{
	int tx_socket = -1;

    if (strcmp(dst_app, get_app_name()) == 0) {
        msgsnd(xmsg_qid, tx_msg, msg_len, 0); //send to self
        return VOS_OK;
    }

    pthread_mutex_lock(&tx_mutex);
    if ( local_is_master ) {
        for (int i = 0; i < MAX_CONNECT_NUM; i++) {
            if (sock_list[i].used == FALSE) {
                continue;
            } else if (strcmp(dst_app, sock_list[i].app_name) == 0) {
                tx_socket = sock_list[i].sock_id;
                sock_list[i].tx_cnt++;	
                break;
            }
        }
    } else {
        tx_socket = local_sock;
    }

    if (tx_socket < 0) {
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    if ( send(tx_socket, tx_msg, msg_len, 0) < msg_len ) {
        xlog_info("send failed(%s)", strerror(errno));
    }

    pthread_mutex_unlock(&tx_mutex);
    return VOS_OK;
}

int app_send_msg(char *dst_app, int msg_type, char *usr_data, int data_len)
{
	char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
    DEVM_MSG_S *tx_msg = (DEVM_MSG_S *)msg_buff;
	static int serial_num = 0;
    
    if (data_len > MSG_MAX_PAYLOAD) {
		xlog_err("data_len %d", data_len);
        return VOS_ERR;
    }

	vos_msleep(10); //avoid packet crush
    memset(msg_buff, 0, sizeof(msg_buff));
    tx_msg->magic_num = htonl(MSG_MAGIC_NUM);
    tx_msg->msg_type = htonl(msg_type);
	tx_msg->serial_num = htonl(serial_num++);
    snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
    snprintf(tx_msg->dst_app, APP_NAME_LEN, "%s", dst_app ? dst_app : "_master");
	xlog_debug("app send to %s", tx_msg->dst_app);
	
    if (usr_data != NULL && data_len != 0) {
        memcpy(tx_msg->msg_payload, usr_data, data_len);
        tx_msg->payload_len = htonl(data_len);
    }

    if (local_is_master && !dst_app) {
        if ( (msg_type < XMSG_T_MAX) && (msg_func_list[msg_type] != NULL) ) {
            msg_func_list[msg_type](tx_msg);
        }
        return VOS_OK;
    }

    return devm_msg_send(tx_msg->dst_app, tx_msg, sizeof(DEVM_MSG_S) + data_len);
}

void* msg_rx_task(void *param)  
{
    int ret;

    while (1) {
		char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
		DEVM_MSG_S *rx_msg = (DEVM_MSG_S *)msg_buff;
        
        int rx_len = msgrcv(xmsg_qid, msg_buff, sizeof(msg_buff), 0, 0);
        if (rx_len < 0) {
            xlog_info("msgrcv %d failed(%s)", xmsg_qid, strerror(errno));
			sleep(1);
            continue;
        }

        if (rx_len < MSG_HEAD_LEN) {
            //xlog(XLOG_ERROR, "invalid msg len %d\n", ret);
            continue;
        }

        xlog_info("new msg %d len %d, sn %u, %s to %s", 
                  ntohl(rx_msg->msg_type), ntohl(rx_msg->payload_len),
                  ntohl(rx_msg->serial_num), rx_msg->src_app, rx_msg->dst_app);
        if ( strcmp(rx_msg->dst_app, "_master") == 0 ) {
            if (!local_is_master) continue;
        } else if ( strcmp(rx_msg->dst_app, get_app_name() ) ) {
            if (local_is_master) { 
                ret = devm_msg_send(rx_msg->dst_app, rx_msg, rx_len);
                xlog_info("forward msg to %s, ret %d", rx_msg->dst_app, ret);
            } else {
                xlog_info("drop msg on wrong route");
            }
            continue;
        }

        rx_msg->magic_num   = ntohl(rx_msg->magic_num);
        rx_msg->msg_type    = ntohl(rx_msg->msg_type);
        rx_msg->serial_num  = ntohl(rx_msg->serial_num);
        rx_msg->payload_len = ntohl(rx_msg->payload_len);
        if ( (rx_msg->msg_type < XMSG_T_MAX) && (msg_func_list[rx_msg->msg_type] != NULL) ) {
            msg_func_list[rx_msg->msg_type](rx_msg);
        }
    }
    
    return NULL;
}

void* socket_rx_task(void *param)  
{
	long temp_val = (long)param;
    int socket_id = (int)temp_val;
	int index;

	xlog(XLOG_INFO, "socket_rx_task %d \n", socket_id);
    //make unblock, if timeout, return EAGAIN or EWOULDBLOCK
    setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &glb_timeout, sizeof(glb_timeout));

    while (1) {
		char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
		DEVM_MSG_S *raw_msg = (DEVM_MSG_S *)msg_buff;
        
        int rx_len = recv(socket_id, msg_buff, sizeof(msg_buff), 0);
        if (rx_len < 0) {
            xlog_info("recv failed(%s)", strerror(errno));
            break;
        } else if (rx_len == 0) {
			vos_msleep(500);
			continue;
        }

        if (rx_len < sizeof(DEVM_MSG_S)) {
            //xlog(XLOG_ERROR, "invalid msg len %d\n", ret);
            continue;
        }

        //xlog(XLOG_DEBUG, "new msg %d", raw_msg->msg_type);
        if (raw_msg->magic_num != ntohl(MSG_MAGIC_NUM)) {
            xlog_info("wrong magic 0x%x", raw_msg->magic_num);
            continue;
        }

		index = devm_update_sock(socket_id, raw_msg->src_app);
		if ( index < 0 ) {
			break;
		}

		sock_list[index].rx_cnt++;
		//xlog_info("new msg %d, %s to %s", raw_msg->msg_type, raw_msg->src_app, raw_msg->dst_app);
        if ( msgsnd(xmsg_qid, raw_msg, rx_len, 0) < 0 ) {
            xlog_info("msgsnd failed(%s)", strerror(errno));
            continue;
        }
    }

    devm_delete_sock(socket_id);
	FORCE_CLOSE(socket_id);
    return NULL;
}

void* inet_listen_task(void *param)  
{
    int listen_fd, ret;
    struct sockaddr_in inet_addr;
	int on = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        xlog_info("socket failed(%s)", strerror(errno));
        return NULL;
    }
	(void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	(void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    
    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family =  AF_INET; 
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(DEF_LISTEN_PORT);
    if (bind(listen_fd,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        xlog_info("bind failed(%s)", strerror(errno));
        return NULL;
    }
    
    if ( listen(listen_fd, MAX_CONNECT_NUM) < 0){
        xlog_info("listen failed(%s)", strerror(errno));
        return NULL;
    }

	xlog_info("%s start, listen_fd %d", __func__, listen_fd);
    while (1) {
        pthread_t unused_tid;
        long int temp_val; //suppress warning
        int new_fd;
        
        new_fd = accept(listen_fd, NULL, NULL);
        if (new_fd < 0) {
            xlog_info("accept failed(%s)", strerror(errno));
            continue;
        }

        xlog(XLOG_INFO, "inet_listen_task new_fd %d", new_fd);
        temp_val = new_fd;
        ret = pthread_create(&unused_tid, NULL, socket_rx_task, (void *)temp_val);  
        if (ret != 0)  {  
            xlog_info("pthread_create failed(%s)", strerror(errno));
            close(new_fd);
            continue;
        } 
    }
    
    close(listen_fd);
    xlog_info("%s exit", __func__);
    return NULL;
}

void* inet_rx_task(void *param)  
{
    char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
    DEVM_MSG_S *raw_msg = (DEVM_MSG_S *)msg_buff;
	int rx_len;

    xlog_info("%s start, local_sock %d", __func__, local_sock);
    while (1) {
		memset(msg_buff, 0, sizeof(msg_buff));
		rx_len = recv(local_sock, msg_buff, sizeof(msg_buff), 0);
		if (rx_len <= 0) {
            //if (rx_len == 0) { usleep(5000); continue; }
			xlog_info("recv failed(%d): %d(%s)", rx_len, errno, strerror(errno));
			break;
		}

        if (raw_msg->magic_num != ntohl(MSG_MAGIC_NUM)) {
            xlog_info("wrong magic 0x%x", raw_msg->magic_num);
            continue;
        }

		//xlog_info("new msg %d, %s to %s", raw_msg->msg_type, raw_msg->src_app, raw_msg->dst_app);
		if (rx_len >= sizeof(DEVM_MSG_S)) {
			msgsnd(xmsg_qid, msg_buff, rx_len, 0);
		}
    }

	xlog_info("%s exit", __func__);
    FORCE_CLOSE(local_sock);
    return NULL;
}

void* inet_tx_task(void *param)  
{
    char msg_buff[MSG_HEAD_LEN];
    DEVM_MSG_S *tx_msg = (DEVM_MSG_S *)msg_buff;
	int tx_len;

    xlog_info("%s start, local_sock %d", __func__, local_sock);
    while (1) {
        if (local_sock < 0) break;
		memset(msg_buff, 0, sizeof(msg_buff));
        tx_msg->magic_num = htonl(MSG_MAGIC_NUM);
        tx_msg->msg_type = htonl(XMSG_T_HELLO);
    	tx_msg->serial_num = 0;
        snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
        snprintf(tx_msg->dst_app, APP_NAME_LEN, "_master");

		tx_len = send(local_sock, tx_msg, MSG_HEAD_LEN, 0);
		if (tx_len <= 0) {
			xlog_info("send failed(%d): %d(%s)", tx_len, errno, strerror(errno));
			break;
		}

		sleep(20);
    }

	xlog_info("%s exit", __func__);
    return NULL;
}

void* inet_connect_task(void *param)
{
    struct sockaddr_in srv_addr;
	int on = 1;
    char *dst_ip;
    pthread_t unused_tid;  
	pthread_attr_t thread_attr;

    xlog_info("%s start, xmsg_server %s", __func__, sys_conf_get("xmsg_server"));
    while (1) {
        dst_ip = sys_conf_get("xmsg_server");
        if ((local_sock >= 0) || (dst_ip == NULL)) {
			sleep(5);
            continue;
        }
        
        local_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (local_sock < 0) {
             xlog_info("socket failed(%d): %s", errno, strerror(errno));
             sleep(10);
             continue;
        }

        srv_addr.sin_family =  AF_INET; 
        srv_addr.sin_port = htons(DEF_LISTEN_PORT);
        srv_addr.sin_addr.s_addr = inet_addr(dst_ip);
        if ( connect(local_sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0  ) {
             //xlog_info("connect failed (%s)", strerror(errno));
             FORCE_CLOSE(local_sock);
             sleep(10);
        } else {
            xlog_info("socket %d connected to %s(%d)", local_sock, dst_ip, DEF_LISTEN_PORT);
            pthread_attr_init(&thread_attr);
            pthread_create(&unused_tid, &thread_attr, (void *)inet_rx_task, NULL);  
            pthread_create(&unused_tid, &thread_attr, (void *)inet_tx_task, NULL);  
        }
    }

	xlog_info("%s exit", __func__);
    return NULL;
}

#endif

#if 1

int echo_msg_proc(DEVM_MSG_S *rx_msg)
{
    vos_print("%s> %s", rx_msg->src_app, rx_msg->msg_payload);
	if (rx_msg->msg_type == XMSG_T_ECHO_REQ) {
		char usr_data[512];
		snprintf(usr_data, 512, "ECHO: %s\n", rx_msg->msg_payload);
    	app_send_msg(rx_msg->src_app, XMSG_T_ECHO_ACK, usr_data, strlen(usr_data) + 1);
	}
    return VOS_OK;
}

int cli_fake_print(void *cookie, char *buff)
{
    DEVM_MSG_S *rx_msg = (DEVM_MSG_S *)cookie;
    char usr_data[512];

    //xlog_info(">> %s \n", buff);
    snprintf(usr_data, 512, "%s", buff);
    app_send_msg(rx_msg->src_app, XMSG_T_ECHO_ACK, usr_data, strlen(usr_data) + 1);
    //vos_msleep(3);
    
    return VOS_OK;
}

int rcmd_msg_proc(DEVM_MSG_S *rx_msg)
{
    xlog_info("rcmd: %s \n", rx_msg->msg_payload);

    cli_set_output_cb(cli_fake_print, rx_msg);
    cli_cmd_exec(rx_msg->msg_payload);
    cli_set_output_cb(NULL, NULL);

    return VOS_OK;
}

int cli_send_echo_msg(int argc, char **argv)
{
    int ret;
    char usr_data[128];

    if (argc < 3) {
        vos_print("usage: %s <dst_app> <str> \r\n", argv[0]);
        return VOS_OK;
    }

    snprintf(usr_data, 128, "%s", argv[2]);
    ret = app_send_msg(argv[1], XMSG_T_ECHO_REQ, usr_data, strlen(usr_data) + 1);
    if (ret != VOS_OK) {  
        vos_print("app_send_msg failed(%d) \n", ret);
        return ret;
    } 

    return VOS_OK;
}

int cli_send_remote_cmd(int argc, char **argv)
{
    int ret;
    char usr_data[256];
    int offset, i;

    if (argc < 3) {
        vos_print("usage: %s <dst_app> ...\r\n", argv[0]);
        return VOS_OK;
    }

    for( i = 2, offset = 0; i < argc; i++)
        offset += sprintf(usr_data + offset, "%s ", argv[i]);

    ret = app_send_msg(argv[1], XMSG_T_RCMD, usr_data, strlen(usr_data) + 1);
    if (ret != VOS_OK) {  
        vos_print("app_send_msg failed(%d) \n", ret);
        return ret;
    } 
    
    return VOS_OK;
}

int cli_show_client_list(int argc, char **argv)
{
    vos_print("client list: \r\n");
    for (int i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            continue;
        }
        vos_print("%04d: app=%s rx_cnt=%d tx_cnt=%d\r\n", 
        		sock_list[i].sock_id, sock_list[i].app_name, sock_list[i].rx_cnt, sock_list[i].tx_cnt);
    }

    return VOS_OK;
}

int devm_msg_init(int is_master)
{
    int ret;
    pthread_t unused_tid;

    xmsg_qid = msgget(IPC_PRIVATE, 0666);
	xlog(XLOG_INFO, "xmsg_qid %d", xmsg_qid);
    if (xmsg_qid == -1) {
        xlog(XLOG_ERROR, "msgget failed(%s)", strerror(errno));
        return VOS_ERR;  
    }

    ret = pthread_create(&unused_tid, NULL, msg_rx_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

    signal(SIGPIPE, SIG_IGN); //ignore socket reset
	local_is_master = is_master;
    if (is_master) {
        ret = pthread_create(&unused_tid, NULL, inet_listen_task, NULL);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
            return VOS_ERR;  
        } 
    } else {
        ret = pthread_create(&unused_tid, NULL, inet_connect_task, NULL);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
            return VOS_ERR;  
        } 
    }

    msg_func_list[XMSG_T_ECHO_REQ]    = echo_msg_proc;
	msg_func_list[XMSG_T_ECHO_ACK]    = echo_msg_proc;
    msg_func_list[XMSG_T_RCMD]        = rcmd_msg_proc;

    cli_cmd_reg("echo",     "send echo msg",            &cli_send_echo_msg);
    if (local_is_master) {
        cli_cmd_reg("rcmd",     "remote cmd",               &cli_send_remote_cmd);
        cli_cmd_reg("list",     "list client",              &cli_show_client_list);
    }

    return VOS_OK;
}

#endif

