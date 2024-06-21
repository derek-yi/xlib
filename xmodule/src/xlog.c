
#include "xmodule.h"

int app_in_master(void);

static char my_log_file[128];

static uint32 xlog_file_max = 0x1000000;

static uint32 my_log_level = XLOG_INFO;

int xlog_cmd_set_level(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("usage: %s get \r\n", argv[0]);
        vos_print("       %s set <log_level> \r\n", argv[0]);
        return VOS_OK;
    }

	if (strncasecmp(argv[1], "set", strlen(argv[1])) == 0) {
		if (argc < 3)  {
	        vos_print("invalid param \r\n");
	        return VOS_E_PARAM;
    	}
		my_log_level = atoi(argv[2]);
	}
	
	vos_print("log_level %d <DEBUG(1) TRACE(2) INFO(3) WARN(4) ERROR(5)> \r\n", my_log_level);
    return VOS_OK;
}

void* xlog_rx_task(void *param)
{
	int udp_sock, rx_len;
    socklen_t peerlen = sizeof(struct sockaddr);
    struct sockaddr_in peer_addr;
    struct sockaddr_in inet_addr;
	char rx_msg[XLOG_BUFF_MAX];
    int level = XLOG_WARN;

    udp_sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (udp_sock < 0) {
        printf("socket failed(%d): %s", errno, strerror(errno));
        return NULL;
    }
    
    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family =  AF_INET; 
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(OAM_XLOG_FWD_PORT);
    if (bind(udp_sock,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        printf("bind failed(%d): %s", errno, strerror(errno));
        return NULL;
    }

    vos_set_self_name("xlog_rx_task");
    while (1) {
		//memset(&rx_msg, 0, sizeof(rx_msg));
        rx_len = recvfrom(udp_sock, rx_msg, XLOG_BUFF_MAX - 1, 0, (struct sockaddr *)&peer_addr, &peerlen);
        if (rx_len <= 0) {
            printf("recvfrom failed(%d): %d(%s)", rx_len, errno, strerror(errno));
            sleep(1);
            continue;
        }
        rx_msg[rx_len] = 0;
        if (rx_msg[0] == '<' && rx_msg[4] == '>') {
            level = (rx_msg[3] - '0')%9;
        }
        _xlog(NULL, 0, XLOG_EXTERN | level, "%s", rx_msg);
    }

    return NULL;
}

int xlog_init(char *log_file)
{
    pthread_t threadid;
    int ret;

    #ifdef MAKE_XLIB
    xlog_file_max = sys_conf_geti("xlog_file_max", 0x1000000);
    #endif

    if (log_file != NULL) {
        snprintf(my_log_file, sizeof(my_log_file), "%s", log_file);
    } else {
        strcpy(my_log_file, "xlog.txt");
    }

    if (app_in_master()) {
        ret = pthread_create(&threadid, NULL, xlog_rx_task, NULL);  
        if (ret != 0) xlog_warn("pthread_create failed(%s)", strerror(errno));
    }
    
    return VOS_OK;
}

int _xlog(const char *func, int line, int level, const char *format, ...)
{
    va_list args;
	char time_str[32];
    char buff[XLOG_BUFF_MAX];
    static int check_cnt = 0;
	int ptr = 0, buf_len;
    char *f_name = func ? (char *)func : "null"; //warning: ‘%s’ directive argument is null

    if ((level & 0xFF) < my_log_level) {
        return VOS_OK;
    } else if (level & XLOG_EXTERN) {
        va_start(args, format);
        ptr = vsnprintf(buff, XLOG_BUFF_MAX - 1, format, args);
        va_end(args);
    } else {
        fmt_time_str(time_str, sizeof(time_str));
        if (app_in_master()) {
        	ptr = snprintf(buff, XLOG_BUFF_MAX, "<MM%d>[%s]<%s,%d> ", level, time_str, f_name, line);
        } else {
        	ptr = snprintf(buff, XLOG_BUFF_MAX, "<SS%d>[%s]<%s,%d> ", level, time_str, f_name, line);
        }
        
        va_start(args, format);
        vsnprintf(buff + ptr, XLOG_BUFF_MAX - 128, format, args);
        va_end(args);
    	strcat(buff, "\r\n");
    }

    buf_len = strlen(buff);
    if (!app_in_master()) {
		if (sys_conf_get("master_ipaddr") != NULL) {
        	vos_send_udp_pkt(sys_conf_get("master_ipaddr"), OAM_XLOG_FWD_PORT, buff, buf_len);
		}
    }
	
    if ( level ) {
        int fd = open(my_log_file, O_RDWR|O_CREAT|O_APPEND, 0666);
        if (fd > 0) {
            write(fd, buff, buf_len);
            //fsync(fd);
            close(fd);
        }
    }

    if (xlog_file_max && (vos_file_size(my_log_file) > xlog_file_max) && (++check_cnt > 100)) {
        check_cnt = 0;
        sprintf(time_str, "/home/log/xlog.init.txt");
        if (vos_file_exist(time_str)) {
            vos_run_cmd("cp -f /home/log/xlog_%d.txt /home/log/xlog.last.txt");
        } else {
            vos_run_cmd("cp -f /home/log/xlog_%d.txt /home/log/xlog.init.txt");
        }
        vos_run_cmd("echo new_log > /home/log/xlog.txt");
    }

    return VOS_OK;    
}



