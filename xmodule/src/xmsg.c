#include "xmodule.h"
#include "xmsg.h"

#define MAX_CONNECT_NUM 32

msg_func msg_func_list[XMSG_T_MAX] = {0};

typedef struct {
    int  sock_id;
    int  used;
    int  ip_addr;
    unsigned int  rx_cnt;
    unsigned int  tx_cnt;
    char app_name[APP_NAME_LEN];
}SOCK_INFO;

SOCK_INFO sock_list[MAX_CONNECT_NUM] = {0};

pthread_mutex_t tx_mutex = PTHREAD_MUTEX_INITIALIZER;

int local_ip_addr = 0;

int local_is_master = 0;

int msg_qid = -1;

#if 1

int get_local_ip(char *if_name)
{
    int inet_sock;  
    struct ifreq ifr;  

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, if_name);  
    if ( ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) {
        return -1;
    }
    
    return (int)((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

int devm_delete_sock(int index)
{
	xlog(XLOG_INFO, "delete sockid %d", index);
	if (index >= MAX_CONNECT_NUM) {
		return VOS_ERR;
	}
	
	close(sock_list[index].sock_id);
	sock_list[index].sock_id = -1;
	sock_list[index].used = FALSE;
	return VOS_OK;
}

int devm_update_sock(int sockid, char *app_name, int ip_addr)
{
	int i, j = -1;
	int match = 0;

	for (i = 0; i < MAX_CONNECT_NUM; i++) {
		if (sock_list[i].used == FALSE) {
			if (j < 0) j = i; //get first unused id
		} else if (sock_list[i].sock_id == sockid) {
			j = i; 
			match = 1;
			break;
		} else if (!strcmp(app_name, sock_list[i].app_name)) {
			j = i; 
			match = 2;
			break;
		}
	}

	if (j < 0) { //full 
		xlog(XLOG_ERROR, "full");
		close(sockid);
		return VOS_ERR;
	}

	if (match == 1) {			
		if (strcmp(sock_list[j].app_name, app_name) != 0) {
			snprintf(sock_list[j].app_name, APP_NAME_LEN, "%s", app_name);
		}
	} else if (match == 2) {
		//todo
	} else {
		xlog(XLOG_INFO, "new sock %d, app_name %s", sockid, app_name);
		sock_list[j].sock_id = sockid;
		snprintf(sock_list[j].app_name, APP_NAME_LEN, "%s", app_name);
		sock_list[j].ip_addr = ip_addr;
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

void* msg_rx_task(void *param)  
{
    while (1) {
		char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
		DEVM_MSG_S *rx_msg = (DEVM_MSG_S *)msg_buff;
        
        int ret = msgrcv(msg_qid, msg_buff, sizeof(msg_buff), 0, 0);
        if (ret < 0) {
            xlog(XLOG_ERROR, "msgrcv failed(%s)", strerror(errno));
            continue;
        }

        //xlog(XLOG_DEBUG, "new msg %d to %s", rx_msg->msg_type, rx_msg->dst_app);
        if ( strcmp(rx_msg->dst_app, get_app_name()) ) {
            if (local_is_master) { 
                ret = devm_msg_forward(rx_msg);
                xlog(XLOG_ERROR, "forward msg to %s, ret %d", rx_msg->dst_app, ret);
            } else {
                xlog(XLOG_ERROR, "drop msg on wrong route");
            }
            continue;
        }
        
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

    while (1) {
		char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
		DEVM_MSG_S *raw_msg = (DEVM_MSG_S *)msg_buff;
        
        int ret = recv(socket_id, msg_buff, sizeof(msg_buff), 0);
        if (ret < 0) {
            xlog(XLOG_ERROR, "recv failed(%s)", strerror(errno));
            break;
        }

        if (ret < sizeof(DEVM_MSG_S)) {
            //xlog(XLOG_ERROR, "invalid msg len %d\n", ret);
            continue;
        }

        //xlog(XLOG_DEBUG, "new msg %d", raw_msg->msg_type);
        if (raw_msg->magic_num != MSG_MAGIC_NUM) {
            xlog(XLOG_ERROR, "wrong magic 0x%x", raw_msg->magic_num);
            continue;
        }

		index = devm_update_sock(socket_id, raw_msg->src_app, raw_msg->src_ip);
		if ( index < 0 ) {
			xlog(XLOG_ERROR, "update_sock: %s 0x%x", raw_msg->src_app, raw_msg->src_ip);
			break;
		}
		sock_list[index].rx_cnt++;
		
        if ( msgsnd(msg_qid, raw_msg, MSG_HEAD_LEN + raw_msg->payload_len, 0) < 0 ) {
            xlog(XLOG_ERROR, "msgsnd failed(%s)", strerror(errno));
            continue;
        }
    }

    return NULL;
}

void* uds_listen_task(void *param)  
{
    int fd, new_fd, ret;
    struct sockaddr_un un;
	int on = 1;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        xlog(XLOG_ERROR, "socket failed(%s)", strerror(errno));
        return NULL;
    }
	(void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "/tmp/sock.%s", get_app_name());
    unlink(un.sun_path);
    if (bind(fd, (struct sockaddr *)&un, sizeof(un)) <0 ) {
        xlog(XLOG_ERROR, "bind failed(%s)", strerror(errno));
        return NULL;
    }
    
    if (listen(fd, MAX_CONNECT_NUM) < 0) {
        xlog(XLOG_ERROR, "listen failed(%s)", strerror(errno));
        return NULL;
    }

    xlog(XLOG_DEBUG, "uds_listen_task: listen %s", get_app_name());
    while(1){
        pthread_t unused_tid;
        long int temp_val; //suppress warning
        
        new_fd = accept(fd, NULL, NULL);
        if (new_fd < 0) {
            xlog(XLOG_ERROR, "accept failed(%s)", strerror(errno));
            continue;
        }

        xlog(XLOG_DEBUG, "uds_listen_task new_fd %d", new_fd);
        temp_val = new_fd;
        ret = pthread_create(&unused_tid, NULL, socket_rx_task, (void *)temp_val);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
            close(new_fd);
            continue;
        } 
    }
    
    close(fd);
    return NULL;
}

void* inet_listen_task(void *param)  
{
    int listen_fd, ret;
    struct sockaddr_in inet_addr;
	int on = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        xlog(XLOG_ERROR, "socket failed(%s)", strerror(errno));
        return NULL;
    }
	(void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family =  AF_INET; 
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(DEF_LISTEN_PORT);
    if (bind(listen_fd,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        xlog(XLOG_ERROR, "bind failed(%s)", strerror(errno));
        return NULL;
    }
    
    if ( listen(listen_fd, MAX_CONNECT_NUM) < 0){
        xlog(XLOG_ERROR, "listen failed(%s)", strerror(errno));
        return NULL;
    }

    while (1) {
        pthread_t unused_tid;
        long int temp_val; //suppress warning
        int new_fd;
        
        new_fd = accept(listen_fd, NULL, NULL);
        if (new_fd < 0) {
            xlog(XLOG_ERROR, "accept failed(%s)", strerror(errno));
            continue;
        }

        xlog(XLOG_DEBUG, "inet_listen_task new_fd %d", new_fd);
        temp_val = new_fd;
        ret = pthread_create(&unused_tid, NULL, socket_rx_task, (void *)temp_val);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
            close(new_fd);
            continue;
        } 
    }
    
    close(listen_fd);
    return NULL;
}

int devm_connect_uds(char *app_name, int *sockid)
{
    int i, j = -1;
    int ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            if (j < 0) j = i;
        }
        else if (!strcmp(app_name, sock_list[i].app_name)) {
			*sockid = sock_list[i].sock_id;;
            return i;
        }
    }

    if (j < 0) { //full
        xlog(XLOG_ERROR, "full");
        return VOS_ERR;
    }

	if (local_is_master) {
        xlog(XLOG_DEBUG, "no connect to %s", app_name);
        return VOS_ERR;
	}
    
    int socket_id = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s)", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "/tmp/sock.%s", app_name);
    ret = connect(socket_id, (struct sockaddr *)&un, sizeof(un));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s)", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

	//xlog(XLOG_INFO, "new sock %d, app_name %s", socket_id, app_name);
    sock_list[j].sock_id = socket_id;
	snprintf(sock_list[j].app_name, APP_NAME_LEN, "%s", app_name);
	sock_list[j].ip_addr = 0;
    sock_list[j].used = TRUE;
	*sockid = socket_id;
    return j;
}

int devm_connect_inet(int dst_ip, int *sockid)
{
    int i, j = -1;
    int ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            if (j < 0) j = i;
        }
        else if (sock_list[i].ip_addr == dst_ip) {
			*sockid = sock_list[i].sock_id;;
            return i;
        }
    }

    if (j < 0) { //full
        xlog(XLOG_ERROR, "sock_list full");
        return VOS_ERR;
    }
    
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s)", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_in dst_addr;
    memset( &dst_addr, 0, sizeof(dst_addr) );
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(DEF_LISTEN_PORT);
    dst_addr.sin_addr.s_addr = dst_ip;
    
    ret = connect(socket_id, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s)", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    sock_list[j].sock_id = socket_id;
    sock_list[j].ip_addr = dst_ip;
    sock_list[j].used = TRUE;
	*sockid = socket_id;
    return j;
}

int devm_msg_forward(DEVM_MSG_S *tx_msg)
{
	int tx_socket, index;
	
    if (tx_msg == NULL) {
        return VOS_ERR;
    }

    pthread_mutex_lock(&tx_mutex);
    index = devm_connect_uds(tx_msg->dst_app, &tx_socket);
    if (index < 0) {
        xlog(XLOG_ERROR, "devm_connect_uds failed, %s", tx_msg->dst_app);
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

	sock_list[index].tx_cnt++;	
    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s)", strerror(errno));
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    pthread_mutex_unlock(&tx_mutex);
    return VOS_OK;
}

int devm_msg_send_local(char *dst_app, DEVM_MSG_S *tx_msg)
{
	int tx_socket, index;

    //xlog(XLOG_DEBUG, "send to %s", dst_app);
    if (tx_msg == NULL) {
        return VOS_ERR;
    }
	
    pthread_mutex_lock(&tx_mutex);
    index = devm_connect_uds(dst_app, &tx_socket);
    if (index < 0) {
        xlog(XLOG_DEBUG, "devm_connect_uds failed");
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    tx_msg->src_ip = 0;
    snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
    snprintf(tx_msg->dst_app, APP_NAME_LEN, "%s", dst_app);
    tx_msg->magic_num = MSG_MAGIC_NUM;

	sock_list[index].tx_cnt++;	
    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s)", strerror(errno));
		devm_delete_sock(index);
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    pthread_mutex_unlock(&tx_mutex);
    return VOS_OK;
}

int devm_msg_send(int dst_ip, char *dst_app, DEVM_MSG_S *tx_msg)
{
	int tx_socket, index;

    if (tx_msg == NULL) {
        return VOS_ERR;
    }
	
    if ( (dst_ip == local_ip_addr) || (dst_ip == 0x0100007f) || (dst_ip == 0) ){
        return devm_msg_send_local(dst_app, tx_msg);
    }

    xlog(XLOG_DEBUG, "send to 0x%x %s", dst_ip, dst_app);
    pthread_mutex_lock(&tx_mutex);
	
    index = devm_connect_inet(dst_ip, &tx_socket);
    if (index < 0) {
        xlog(XLOG_ERROR, "devm_connect_inet failed");
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    tx_msg->src_ip = local_ip_addr;
    snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
    snprintf(tx_msg->dst_app, APP_NAME_LEN, "%s", dst_app);
    tx_msg->magic_num = MSG_MAGIC_NUM;

	sock_list[index].tx_cnt++;	
    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s)", strerror(errno));
		devm_delete_sock(index);
        pthread_mutex_unlock(&tx_mutex);
        return VOS_ERR;
    }

    pthread_mutex_unlock(&tx_mutex);
    return VOS_OK;
}

int app_send_msg(int dst_ip, char *dst_app, int msg_type, char *usr_data, int data_len)
{
	char msg_buff[MSG_MAX_PAYLOAD + MSG_HEAD_LEN];
    DEVM_MSG_S *tx_msg = (DEVM_MSG_S *)msg_buff;
    
    if (dst_app == NULL || data_len > MSG_MAX_PAYLOAD) {
        return VOS_ERR;
    }

    //xlog(XLOG_DEBUG, "app send to 0x%x %s", dst_ip, dst_app);
    memset(msg_buff, 0, sizeof(msg_buff));
    tx_msg->msg_type = msg_type;
	
    if (usr_data != NULL && data_len != 0) {
        memcpy(tx_msg->msg_payload, usr_data, data_len);
        tx_msg->payload_len = data_len;
    }

    return devm_msg_send(dst_ip, dst_app, tx_msg);
}

#endif

#if 1

int echo_msg_proc(DEVM_MSG_S *rx_msg)
{
    vos_print("%s: %s\n", rx_msg->src_app, rx_msg->msg_payload);

	if (rx_msg->msg_type == XMSG_T_ECHO_REQ) {
		char usr_data[64];
		sprintf(usr_data, "ECHO_ACK");
    	app_send_msg(rx_msg->src_ip, rx_msg->src_app, XMSG_T_ECHO_ACK, usr_data, strlen(usr_data) + 1);
	}
    return VOS_OK;
}

int cli_fake_print(void *cookie, char *buff)
{
    DEVM_MSG_S *rx_msg = (DEVM_MSG_S *)cookie;
    char usr_data[512];

    //xlog(XLOG_DEBUG, "%d: %s \n", __LINE__, buff);
    snprintf(usr_data, 512, "%s", buff);
    app_send_msg(rx_msg->src_ip, rx_msg->src_app, XMSG_T_ECHO_ACK, usr_data, strlen(usr_data) + 1);
    vos_msleep(3);
    
    return VOS_OK;
}

int rcmd_msg_proc(DEVM_MSG_S *rx_msg)
{
    xlog(XLOG_DEBUG, "rcmd_msg_proc %s \n", rx_msg->msg_payload);

    cli_set_output_cb(cli_fake_print, rx_msg);
    cli_cmd_exec(rx_msg->msg_payload);
    cli_set_output_cb(NULL, NULL);

    return VOS_OK;
}

int cli_send_local_echo(int argc, char **argv)
{
    int ret;
    char usr_data[128];

    if (argc < 3) {
        vos_print("usage: %s <dst_app> <str> \r\n", argv[0]);
        return VOS_OK;
    }

    snprintf(usr_data, 128, "msg(%s)", argv[2]);
    ret = app_send_msg(0, argv[1], XMSG_T_ECHO_REQ, usr_data, strlen(usr_data) + 1);
    if (ret != VOS_OK) {  
        vos_print("app_send_msg failed(%d) \n", ret);
        return ret;
    } 

    return VOS_OK;
}

int cli_send_remote_echo(int argc, char **argv)
{
    int ret;
    char usr_data[128];
    int dst_ip;

    if (argc < 4) {
        vos_print("usage: %s <dst_ip> <dst_app> <str> \r\n", argv[0]);
        return VOS_OK;
    }

    snprintf(usr_data, 128, "msg(%s)", argv[3]);
    inet_pton(AF_INET, argv[1], &dst_ip);
    
    ret = app_send_msg(dst_ip, argv[2], XMSG_T_ECHO_REQ, usr_data, strlen(usr_data) + 1);
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
    int dst_ip;
    int offset, i;

    if (argc < 4) {
        vos_print("usage: %s <dst_ip> <dst_app> ...\r\n", argv[0]);
        return VOS_OK;
    }

    for( i = 3, offset = 0; i < argc; i++)
        offset += sprintf(usr_data + offset, "%s ", argv[i]);

    inet_pton(AF_INET, argv[1], &dst_ip);
    ret = app_send_msg(dst_ip, argv[2], XMSG_T_RCMD, usr_data, strlen(usr_data) + 1);
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
        vos_print("%04d: ip=0x%08x app=%s rx_cnt=%d tx_cnt=%d\r\n", 
        		sock_list[i].sock_id, sock_list[i].ip_addr, sock_list[i].app_name, sock_list[i].rx_cnt, sock_list[i].tx_cnt);
    }

    return VOS_OK;
}

int devm_msg_init(char *app_name, int master)
{
    int ret;
    char *cfg_str;
    pthread_t unused_tid;

    cfg_str = sys_conf_get("eth_name");
    if (cfg_str) {
        local_ip_addr = get_local_ip(cfg_str);
        xlog(XLOG_INFO, "local_ip_addr 0x%x", local_ip_addr);
    }

    msg_qid = msgget(IPC_PRIVATE, 0666);
    if (msg_qid == -1) {
        xlog(XLOG_ERROR, "msgget failed(%s)", strerror(errno));
        return VOS_ERR;  
    }

    ret = pthread_create(&unused_tid, NULL, msg_rx_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

    signal(SIGPIPE, SIG_IGN); //ignore socket reset
    ret = pthread_create(&unused_tid, NULL, uds_listen_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

	local_is_master = master;
    if (master) {
        ret = pthread_create(&unused_tid, NULL, inet_listen_task, NULL);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
            return VOS_ERR;  
        } 
    }

    msg_func_list[XMSG_T_ECHO_REQ]    = echo_msg_proc;
	msg_func_list[XMSG_T_ECHO_ACK]    = echo_msg_proc;
    msg_func_list[XMSG_T_RCMD]        = rcmd_msg_proc;

    cli_cmd_reg("echo",     "send local echo",          &cli_send_local_echo);
    //cli_cmd_reg("tx",       "send remote echo",         &cli_send_remote_echo);
    //cli_cmd_reg("rcmd",     "remote cmd",               &cli_send_remote_cmd);
    cli_cmd_reg("list",     "list client",              &cli_show_client_list);

    return VOS_OK;
}

#endif

