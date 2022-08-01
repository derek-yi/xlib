#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>   //time_t
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "vos.h"    //vos_run_cmd
#include "syscfg.h"
#include "xlog.h"

//#define INCLUDE_SYSLOG

#ifndef MAKE_XLIB
#define vos_print   printf
#endif

static char my_log_file[128];

static uint32 my_log_level = XLOG_INFO;

static uint32 my_print_level = XLOG_ERROR;

#ifdef INCLUDE_SYSLOG

int syslog_sock = -1;

int syslog_sendmsg(void *tx_buff, int len) 
{
	int ret;
	char *syslog_ip;
	static struct sockaddr_in srv_addr;

	syslog_ip = sys_conf_get("ftp_ipaddr");
	if ( syslog_ip == NULL) return 0;
	
	if (syslog_sock < 0) {
		syslog_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (syslog_sock < 0) {
			///printf("invalid xlog_sock %d", syslog_sock);
			return -1;
		}
		memset(&srv_addr, 0, sizeof(srv_addr));
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_port = htons(514);
		srv_addr.sin_addr.s_addr = inet_addr(syslog_ip);
	}

	ret = sendto(syslog_sock, tx_buff, len, 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
	if (ret <= 0) {
		//printf("sendto failed %d", ret);
		return -1;
	}

	return 0;
}
#endif

int xlog_print_file(char *filename)
{
    FILE *fp;
    char temp_buf[XLOG_BUFF_MAX];
    const char *cp_file = "temp_file";
    
    sprintf(temp_buf, "cp -f /var/log/%s %s", filename, cp_file);
    vos_run_cmd(temp_buf);
    
    fp = fopen(cp_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "open failed, %s\n", strerror(errno));
        unlink(cp_file);
        return VOS_ERR;
    }

    vos_print("%s: \r\n", filename);
    memset(temp_buf, 0, sizeof(temp_buf));
    while (fgets(temp_buf, XLOG_BUFF_MAX-1, fp) != NULL) {  
        vos_print("%s\r", temp_buf); //linux-\n, windows-\n\r
        memset(temp_buf, 0, sizeof(temp_buf));
    }

    fclose(fp);
    unlink(cp_file);
    return VOS_OK;
}

int xlog_set_level(uint32 log_level, uint32 print_level)
{
    my_log_level = log_level;
    my_print_level = print_level;
    return 0;
}

int cli_xlog_level(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("usage: %s get \r\n", argv[0]);
        vos_print("       %s set <log_level> <print_level> \r\n", argv[0]);
        return VOS_OK;
    }

	if (strncasecmp(argv[1], "set", strlen(argv[1])) == 0) {
		if (argc < 4)  {
	        vos_print("invalid param \r\n");
	        return VOS_E_PARAM;
    	}

		xlog_set_level(atoi(argv[2]), atoi(argv[3]));
	}
	
	vos_print("log_level %d, print_level %d\r\n", my_log_level, my_print_level);
	vos_print("=> DEBUG(1) INFO(2) WARN(3) ERROR(4)\r\n");
    return VOS_OK;
}

int xlog_init(char *log_file)
{
    if (log_file != NULL) {
        snprintf(my_log_file, sizeof(my_log_file), "%s", log_file);
    } else {
        strcpy(my_log_file, "xlog.txt");
    }

    return VOS_OK;
}

int _xlog(const char *func, int line, int level, const char *format, ...)
{
    va_list args;
	char time_str[32];
    char buff[XLOG_BUFF_MAX];
	int ptr = 0;

	if ( (level < my_print_level) && (level < my_log_level) ) {
		return VOS_OK;
	}
	
	fmt_time_str(time_str, sizeof(time_str));
	#ifdef INCLUDE_SYSLOG
	ptr = sprintf(buff, "<1>[%s]<%s,%d> ", time_str, func, line);
	#else
	ptr = sprintf(buff, "[%s]<%s,%d> ", time_str, func, line);
	#endif
    va_start(args, format);
    vsnprintf(buff + ptr, XLOG_BUFF_MAX - ptr, format, args);
    va_end(args);
	strcat(buff, "\r\n");
	
    if ( level >= my_print_level ) {
        vos_print("%s", buff);
    } 

    if ( level >= my_log_level ) {
        int fd = open(my_log_file, O_RDWR|O_CREAT|O_APPEND, 0666);
		int buf_len = strlen(buff);
        if (fd > 0) {
            write(fd, buff, buf_len);
            close(fd);
        }
		#ifdef INCLUDE_SYSLOG
		syslog_sendmsg(buff, buf_len);
		#endif
    } 

    return VOS_OK;    
}

#ifndef MAKE_XLIB
//undef INCLUDE_SYSLOG
//gcc xlog.c vos.c -I../include -lpthread -lrt
int main()
{	
	xlog_set_level(XLOG_INFO, XLOG_WARN);
	xlog_init("./my_log.txt");

	xlog_debug("this is xlog_debug");
	xlog_info("this is xlog_info");
	xlog_warn("this is xlog_warn");
	xlog_err("this is xlog_err");

    return VOS_OK;
}
#endif

