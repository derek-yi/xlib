#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <stdarg.h>
#include <termios.h>

#ifndef DEMO_APP
#include "xmodule.h"
#endif

#define CMD_MAX_LEN                     512
#define CMD_TYPE_CASIC                  0
#define CMD_TYPE_NMEA                   1
#define CMD_TYPE_UBLOX                  2
#define PL_START                        6
#define MAX_SAT_NUM                     100
#define UBX_PL_START                    6

#define NMEA_HEAD                       0x24 //$
#define CASIC_HEAD1                     0xBA
#define CASIC_HEAD2                     0xCE
#define UBLOX_HEAD1                     0xB5
#define UBLOX_HEAD2                     0x62

#define RX_ST_WAIT_HEAD					0x0
#define RX_ST_WAIT_LEN					0x1
#define RX_ST_NMEA_DATA					0x2
#define RX_ST_CASIC_DATA				0x3
#define RX_ST_UBLOX_DATA				0x4

#define CMD_PCAS00_SAVE_CFG             0x10000
#define CMD_PCAS01_BAUD                 0x10001
#define CMD_PCAS02_LOC_FREQ             0x10002
#define CMD_PCAS03_OUT_CFG              0x10003
#define CMD_PCAS04_GNSS_CFG             0x10004
#define CMD_PCAS06_GET_VERSION          0x10006
#define CMD_PCAS10_REBOOT               0x10010

#define CMD_CASIC_ACK_PASS              0x0501
#define CMD_CASIC_ACK_FAIL              0x0500
#define CMD_CASIC_CFG_PRT               0x0600

typedef struct
{
	long q_resv;
    int  type;
	int  len;
	uint8_t data[CMD_MAX_LEN];
}wrap_msg; // for msgq

typedef int (* rx_msg_cb)(wrap_msg *ack_msg);

#ifdef DEMO_APP

#define VOS_OK      		    0
#define VOS_ERR     		    (-1)
#define VOS_E_PARAM     	    (-2)
#define VOS_E_TIMEOUT     	    (-3)

#define vos_print               printf
#define vos_msleep(x)           usleep((x)*1000)

#define XLOG_BUFF_MAX           1024

static int my_debug_level = 0x3;
static char my_log_file[128];

/* mutex for xlog */
sem_t xlog_sem;	

int cli_param_format(char *param, char **argv, int max_cnt)
{
    char *ptr = param;
    int cnt = 0;
    int flag = 0;

    while(*ptr == 0)
    {
        if(*ptr != ' ' && *ptr != '\t')
            break;
        ptr++;
    }

    if(*ptr == 0) return 0;
    argv[cnt++] = ptr;
    flag = 1;

    while(cnt < max_cnt && *ptr != 0)
    {
        if(flag == 0 && *ptr != '\t' && *ptr != ' ')
        {
            argv[cnt++] = ptr;
            flag = 1;
        }
        else if(flag == 1 && (*ptr == ' ' || *ptr == '\t'))
        {
            flag = 0;
            *ptr = 0;
        }
        ptr++;
    }

    //get the last param
    if(*ptr != 0)
    {   
        while(*ptr != 0)
        {
            if(*ptr == ' ' || *ptr == '\t')
            {   
                *ptr = 0;
                break;
            }
            ptr++;
        }
    }

    return cnt;
}

int vos_sem_wait(void *sem_id, uint32_t msecs)
{
	struct timespec ts;
    sem_t *sem = (sem_t *)sem_id;
	uint32_t add = 0;
	uint32_t secs = msecs/1000;
    
	clock_gettime(CLOCK_REALTIME, &ts);

	msecs = msecs%1000;
	msecs = msecs*1000*1000 + ts.tv_nsec;
	add = msecs / (1000*1000*1000);
	ts.tv_sec += (add + secs);
	ts.tv_nsec = msecs%(1000*1000*1000);
 
	return sem_timedwait(sem, &ts);
}

int vos_sem_clear(void *sem_id)
{
	int cnt, ret;

    if (sem_id == NULL) return 1;
	while (1) {
        cnt = 0;
        ret = sem_getvalue(sem_id, &cnt);
        if ((ret < 0) || (cnt < 1)) break;
        sem_trywait(sem_id);
    }
 
	return 0;
}

int tty_set_raw(int fd, int baud)
{
	struct termios newtio;

	tcgetattr(fd, &newtio); /* save current serial port settings */

	/* https://stackoverflow.com/questions/39576885/binary-byte-mistaken-with-newline-in-linux-serial-comunication */
	//cfmakeraw(&newtio);
	//Set raw mode, equivalent to:
	newtio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
				| INLCR | IGNCR | ICRNL | IXON);
	newtio.c_oflag &= ~OPOST;
	newtio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

	/* 
	initialize all control characters 
	default values can be found in /usr/include/termios.h, and are given
	in the comments, but we don't need them here
	*/
	newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	newtio.c_cc[VERASE]   = 0;     /* del */
	newtio.c_cc[VKILL]    = 0;     /* @ */
	newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	newtio.c_cc[VSWTC]    = 0;     /* '\0' */
	newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	newtio.c_cc[VEOL]     = 0;     /* '\0' */
	newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	newtio.c_cc[VEOL2]    = 0;     /* '\0' */

	//set baud rate
	if (baud == 0) {
    	cfsetispeed(&newtio, B9600);
	    cfsetospeed(&newtio, B9600);
    } else {
    	cfsetispeed(&newtio, B115200);
	    cfsetospeed(&newtio, B115200);
    }
	
	/* 
	now clean the modem line and activate the settings for the port
	*/
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);  /* update it now */ 
	
	return 0;
}

void fmt_time_str(char *time_str, int max_len)
{
    struct tm *tp;
    time_t t = time(NULL);
    tp = localtime(&t);
     
    if (!time_str) return ;
    
    snprintf(time_str, max_len, "%02d%02d_%02d_%02d_%02d", 
            tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

int xlog_init(uint32_t dbg_level)
{
	snprintf(my_log_file, sizeof(my_log_file), "%s", "./log_gnss_mng.txt");
	unlink(my_log_file);

	my_debug_level = dbg_level;
	sem_init(&xlog_sem, 0, 1);
	return 0;
}

int _mylog(int line, char *format, ...)
{
	va_list args;
	char time_str[32];
	char buff[XLOG_BUFF_MAX];
	int len;

	if (my_debug_level == 0) return 0;

	sem_wait(&xlog_sem);
	fmt_time_str(time_str, sizeof(time_str));
	len = sprintf(buff, "[%s]<%d> ", time_str, line);

	va_start(args, format);
	len += vsnprintf(buff + len, XLOG_BUFF_MAX - len, format, args);
	va_end(args);
	strcat(buff, "\r\n");

    int fd = open(my_log_file, O_RDWR|O_CREAT|O_APPEND, 0666);
    if (fd > 0) {
        write(fd, buff, strlen(buff));
        close(fd);
    }

	sem_post(&xlog_sem);
	return len;    
}

#define xlog_warn(...)  	do { if (my_debug_level & 0x1) _mylog(__LINE__, __VA_ARGS__); } while(0)
#define xlog_info(...)  	do { if (my_debug_level & 0x2) _mylog(__LINE__, __VA_ARGS__); } while(0)
#define xlog_debug(...)  	do { if (my_debug_level & 0x4) _mylog(__LINE__, __VA_ARGS__); } while(0)

#endif

#if 1

typedef struct
{
    //mng
    int module_type;
    uint64_t GSV_cnt;
    uint64_t GGA_cnt;
    uint64_t GLL_cnt;
    uint64_t RMC_cnt;
    uint64_t ZDA_cnt;
    uint64_t GSA_cnt;
    
    //from RMC
    struct tm utc_tm;
    uint8_t status; //A-valid, V-invalid
    float latitude;
    float lontitude;

    //FROM GGA
    uint8_t Quality;
    uint16_t NumSatUsed;

    //from GSA
    uint8_t pos_type;
}GPS_INFO;

typedef struct
{
    int gnss_type; //0-null, 1-gps, 2-beidou, 3-gloss
    int timeout;
    int svid;
    int elevation;
    int azimuth;
    int cno;
}SAT_INFO;

GPS_INFO gps_info;
SAT_INFO sat_info[256];

int tty_fd = -1;

static int rx_msg_qid = -1;

sem_t sync_sem; 

pthread_mutex_t cmd_mtx;

int pending_cmd = 0;

wrap_msg ack_msg;

int module_type = 1;  //1-atg, 0-ublox
    
int gps_debug = 0;

int my_sscanf(uint8_t *buff, int max, char **sub)
{
    int cnt = 0;
    int buf_len = strlen(buff);
    uint8_t *start = buff;

    for (int i = 0; i < buf_len; i++) {
        if (buff[i] == ',' || buff[i] == '*') {
            buff[i] = 0; //replace to \0
            
            if (*start == 0) {
                //xlog_debug(">> %d is null", cnt);
                sub[cnt++] = NULL;
            } else {
                //xlog_debug(">> %d is %s", cnt, start);
                sub[cnt++] = start;
            }

            start = &buff[i + 1];
            if (cnt == max) return cnt;
        }
    }

    return cnt;
}

int add_new_sat(SAT_INFO *sat)
{
    int i, j;

    if (sat == NULL) return VOS_ERR;
    sat->timeout = 20;
    for (i = 0, j = -1; i < MAX_SAT_NUM; i++) {
        if (sat_info[i].gnss_type == 0) {
            if (j < 0) j = i;
        } else if ( (sat_info[i].gnss_type == sat->gnss_type) 
            && (sat_info[i].svid == sat->svid) ) {
            memcpy(&sat_info[i], sat, sizeof(SAT_INFO));
            return VOS_OK;
        }
    }

    if (j < 0) return VOS_ERR;
    memcpy(&sat_info[j], sat, sizeof(SAT_INFO));
    return VOS_OK;
}

int sat_list_age(void)
{
    for (int i = 0; i < MAX_SAT_NUM; i++) {
        if (sat_info[i].gnss_type) {
            if (sat_info[i].timeout-- < 0) {
                sat_info[i].gnss_type = 0;
            }
        }
    }

    return VOS_OK;
}

int GNGSV_Parse(uint8_t *buff, int len)
{
    char *sub[32];
    int value;
    SAT_INFO sat;
    int gnss_type = 3;

    if (memcmp(buff, "$BDGSV", 6) == 0) gnss_type = 2; //beidou
    if (memcmp(buff, "$GBGSV", 6) == 0) gnss_type = 2; //beidou
    else if (memcmp(buff, "$GPGSV", 6) == 0) gnss_type = 1; //gps
    
    //可见卫星的卫星编号及其仰角、 方位角、 载噪比等信息
    //$--GSV,numMsg,msgNo,numSv{,SVID,ele,az,cn0} *CS<CR><LF>
    //$BDGSV,5,1,17,01,46,123,38,04,32,111,40,07,23,174,29,08,59,197,,1*7B
    //$BDGSV,5,5,17,46,13,039,39,1*43
    value = my_sscanf(buff, 32, sub);
    if (value < 10) return VOS_ERR;
    sat.gnss_type = gnss_type;
    for (int i = 4; i < value - 2; i += 4) {
        if (sub[i] == NULL) continue;
        sat.svid = atoi(sub[i]);
        sat.elevation = (sub[i + 1] != NULL) ? atoi(sub[i + 1]) : 0;
        sat.azimuth = (sub[i + 2] != NULL) ? atoi(sub[i + 2]) : 0;
        sat.cno = (sub[i + 3] != NULL) ? atoi(sub[i + 3]) : 0;
        add_new_sat(&sat);
    }

    gps_info.GSV_cnt++;
    return VOS_OK;
}

int GNGGA_Parse(uint8_t *buff, int len)
{
    char *sub[16];
    int value;

    //接收机时间、 位置及定位相关的数据
    //$--GGA,UTCtime,lat,uLat,lon,uLon,FS,numSv,HDOP,msl,uMsl,sep,uSep,diffAge,diffSta*CS<CR><LF>
    //$GNGGA,090157.00,2231.52989,N,11355.98741,E,1,20,0.8,150.13,M,-3.54,M,,*66
    //$GPGGA,235316.000,2959.9925,S,12000.0090,E,1,06,1.21,62.77,M,0.00,M,,*7B
    value = my_sscanf(buff, 13, sub);
    if (value < 13) return VOS_ERR;
    //xlog_debug("value %d: %s %s %s", value, sub[0], sub[1], sub[13]);

    if (sub[6]) {
        value = atoi(sub[6]);
        gps_info.Quality = value;
    }

    if (sub[7]) {
        value = atoi(sub[7]);
        gps_info.NumSatUsed = value;
    }

    gps_info.GGA_cnt++;
    return VOS_OK;
}

int GNGLL_Parse(uint8_t *buff, int len)
{
    //纬度、 经度、 定位时间与定位状态等信息。
    //$--GLL,lat,uLat,lon,uLon, UTCtime,valid,mode*CS<CR><LF>
    //$GNGLL,2231.52989,N,11355.98741,E,090157.00,A,A*70
    //ublox $GPGLL,4717.11364,N,00833.91565,E,092321.00,A,A*60
    gps_info.GLL_cnt++;
    return VOS_OK;
}

int GNGSA_Parse(uint8_t *buff, int len)
{
    char *sub[16];
    int value;

    //用于定位的卫星编号与 DOP 信息。 
    //$--GSA,smode,FS{,SVID},PDOP,HDOP,VDOP*CS<CR><LF>
    //$GNGSA,A,3,12,18,21,23,25,31,,,,,,,1.2,0.8,1.0,1*36
    //$GNGSA,A,3,01,04,07,10,14,24,33,38,40,42,,,1.2,0.8,1.0,4*37
    //$GBGSA,A,3,,,,,,,,,,,,,2.53,1.35,2.14*14
    value = my_sscanf(buff, 8, sub);
    if (value < 8) return VOS_ERR;
    if (sub[1] == NULL || sub[2] == NULL) return VOS_ERR;
    //xlog_debug("value %d: %s %s %s", value, sub[0], sub[1], sub[13]);
    
    gps_info.pos_type = atoi(sub[2]); //1-no-fix, 2-2d-fix, 3-3d-fix
    if (gps_debug > 1) {
        xlog_debug("Quality %d, status %c, pos_type %d",
               gps_info.Quality, gps_info.status, gps_info.pos_type);
    }

    gps_info.GSA_cnt++;
    return VOS_OK;
}

int GNRMC_Parse(uint8_t *buff, int len)
{
    char *sub[16];
    int value;
    float f_value;

    //推荐的最小定位信息
    //$--RMC,UTCtime,status,lat,uLat,lon,uLon,spd,cog,date,mv,mvE,mode*CS<CR><LF>
    //$GNRMC,,V,,,,,,,,,,N,V*37
    //$GNRMC,090158.00,A,2231.52987,N,11355.98738,E,0.27,,080923,,,A,V*29
    //$BDRMC,080513.00,A,2231.52932,N,11355.98552,E,0.47,,130923,,,A,V*20
    //$GNRMC,080130.00,V,,,,,,,130923,,,N*63
    //$GBRMC,093247.00,A,2231.51155,N,11356.04006,E,0.150,,140923,,,A*65
    value = my_sscanf(buff, 13, sub);
    if (value < 13) return VOS_ERR;
    if (sub[1] == NULL || sub[2] == NULL) return VOS_ERR;
    //xlog_debug("value %d: %s %s %s", value, sub[0], sub[1], sub[13]);

    value = atoi(sub[1]);  //061713.000
    gps_info.utc_tm.tm_hour = value/10000;
    gps_info.utc_tm.tm_min  = (value%10000)/100;
    gps_info.utc_tm.tm_sec  = (value%100);

    gps_info.status = sub[2][0]; //A

    if (sub[3]) {
        f_value = atof(sub[3]); //2231.52553
        gps_info.latitude = f_value;
    }

    if (sub[5]) {
        f_value = atof(sub[5]); //11355.99090
        gps_info.lontitude = f_value;
    }

    if (sub[9]) {
        value = atoi(sub[9]); //080923
        gps_info.utc_tm.tm_year = (value%100) + 100;
        gps_info.utc_tm.tm_mon  = (value%10000)/100 - 1;
        gps_info.utc_tm.tm_mday  = value/10000;
    }

    gps_info.RMC_cnt++;
    return VOS_OK;
}


int GNZDA_Parse(uint8_t *buff, int len)
{
    //时间与日期信息
    //$GNZDA,090157.00,08,09,2023,00,00*70
    gps_info.ZDA_cnt++;
    return VOS_OK;
}

int GPTXT_Parse(uint8_t *buff, int len)
{
    //产品信息,开机时输出一次
    //$GPTXT,01,01,01,ANTENNA OPEN*25
    return VOS_OK;
}

void msg_rx_thread(void *param)  
{
    wrap_msg rx_msg;

    vos_set_self_name("gnss_msg_rx_task");
    xlog_info("rx_msg_qid %d ", rx_msg_qid);
    while (1)  {
        if (msgrcv(rx_msg_qid, &rx_msg, sizeof(rx_msg), 0, 0) == -1) {
            xlog_warn("msgrcv error");
			sleep(1);
            continue;
        }

        //xlog_debug("type %d, len %d, pending_cmd 0x%x", rx_msg.type, rx_msg.len, pending_cmd);
        if (rx_msg.type == CMD_TYPE_NMEA) {
            if (gps_debug > 1) xlog_debug("NMEA(%d): %s", rx_msg.len, rx_msg.data);
            if (memcmp(&rx_msg.data[3], "GSV", 3) == 0) GNGSV_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "GGA", 3) == 0) GNGGA_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "GLL", 3) == 0) GNGLL_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "GSA", 3) == 0) GNGSA_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "TXT", 3) == 0) GPTXT_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "ZDA", 3) == 0) GNZDA_Parse(rx_msg.data, rx_msg.len);
            else if (memcmp(&rx_msg.data[3], "RMC", 3) == 0) {
                GNRMC_Parse(rx_msg.data, rx_msg.len);
                sat_list_age();
            } 
        } else {
            if (gps_debug) {
                int i, ptr;
                char line_str[128];
                xlog_info("MSG(%d): ", rx_msg.len);
                for (i = 0, ptr = 0; i < rx_msg.len; i++) {
                    ptr += sprintf(line_str + ptr, "%02x ", rx_msg.data[i]);
                    if ( (i + 1)%16 == 0 ) {
                        xlog_info("%s", line_str);
                        ptr = 0;
                    }
                } 
                if (ptr) xlog_info("%s", line_str);
            }

            if (rx_msg.type == CMD_TYPE_CASIC) {
                //ba ce 08 00 06 00 00 33 00 00 00 c2 01 00 08 f5
                if ( (rx_msg.data[4] == (pending_cmd >> 8)) 
                    && (rx_msg.data[5] == (pending_cmd & 0xFF)) ) {
                    if (rx_msg.len > 0xFF) xlog_info("rx_msg.len %d error", rx_msg.len);
                    memcpy(&ack_msg, &rx_msg, sizeof(wrap_msg));
                    xlog_info("ack_msg %d", ack_msg.len);
                    sem_post(&sync_sem);
                }
            } else {
                //0xB5 0x62 0x0A 0x04 <40 + 30*N> <payload> CK_A CK_B
                if ( (rx_msg.data[2] == (pending_cmd >> 8)) 
                    && (rx_msg.data[3] == (pending_cmd & 0xFF)) ) {
                    if (rx_msg.len > 0xFF) xlog_info("rx_msg.len %d error", rx_msg.len);
                    memcpy(&ack_msg, &rx_msg, sizeof(wrap_msg));
                    if (gps_debug) xlog_info("ack_msg %d", ack_msg.len);
                    sem_post(&sync_sem);
                }
            }
        } 
    }
}

void tty_rx_thread(void *param)  
{
	int ret;
	int rx_state = RX_ST_WAIT_HEAD;
	int rx_ptr = 0;
	wrap_msg rx_msg;
	uint8_t rx_byte;
	int pl_len;

    vos_set_self_name("gnss_tty_rx_task");
    xlog_info("tty_fd %d", tty_fd);
	while (1) {
		ret = read(tty_fd, &rx_byte, 1);
		if (ret < 0) {
			sleep(1);
			continue;
		}

        //xlog_debug("rx_state %d, rx_byte 0x%x", rx_state, rx_byte);
		switch (rx_state) {
			case RX_ST_WAIT_HEAD:
				if (rx_byte == NMEA_HEAD) {
					//xlog_debug("new NMEA msg");
					rx_msg.data[0] = NMEA_HEAD;
                    rx_state = RX_ST_NMEA_DATA;
                    rx_ptr = 1;
				} else if (rx_byte == CASIC_HEAD1) {
                    xlog_debug("CASIC_HEAD1");
					rx_msg.data[0] = CASIC_HEAD1;
				} else if (rx_byte == CASIC_HEAD2) {
					rx_msg.data[1] = CASIC_HEAD2;
                    if (rx_msg.data[0] == CASIC_HEAD1) {
                        xlog_debug("CASIC_HEAD2");
                        rx_state = RX_ST_CASIC_DATA;
                        pl_len = CMD_MAX_LEN; //init
                        rx_ptr = 2;
                    }
				}  else if (rx_byte == UBLOX_HEAD1) {
                    //if (gps_debug) xlog_debug("UBLOX_HEAD1");
					rx_msg.data[0] = UBLOX_HEAD1;
				} else if (rx_byte == UBLOX_HEAD2) {
					rx_msg.data[1] = UBLOX_HEAD2;
                    if (rx_msg.data[0] == UBLOX_HEAD1) {
                        //if (gps_debug) xlog_debug("UBLOX_HEAD2");
                        rx_state = RX_ST_UBLOX_DATA;
                        pl_len = CMD_MAX_LEN; //init
                        rx_ptr = 2;
                    }
				} else {
				    //xlog_debug("rx_byte 0x%x", rx_byte);
                }
				break;
				
			case RX_ST_NMEA_DATA:
				rx_msg.data[rx_ptr++] = rx_byte;
				if (rx_byte == '\n') {
                    if (rx_msg.data[rx_ptr - 2] != '\r') {
                        xlog_warn("wrong NMEA msg");
                        char line_str[128];
                        int ptr = 0;
                        for (int i = 0; i < rx_ptr; i++) {
                            ptr += sprintf(line_str + ptr, "%02x ", (uint8_t)rx_msg.data[i]);
                        } 
                        xlog_debug("%s", line_str);
                    } else {
                        //send to msg_rx_thread
                        rx_msg.q_resv = 10; //magic
                        rx_msg.type = CMD_TYPE_NMEA;
                        rx_msg.len = rx_ptr - 2;
                        rx_msg.data[rx_ptr - 2] = 0; // \r to \0
                        ret = msgsnd(rx_msg_qid, &rx_msg, sizeof(wrap_msg), 0);
                        //xlog_debug("msgsnd %d, ret %d", rx_msg.len, ret);
                    }
                    rx_state = RX_ST_WAIT_HEAD;
                } else if (rx_ptr == CMD_MAX_LEN) {
                    xlog_warn("NMEA msg overflow");
                    rx_state = RX_ST_WAIT_HEAD;
                } else if (rx_byte > 127) {
                    //not printable char
                    rx_state = RX_ST_WAIT_HEAD;
                }
				break;
				
			case RX_ST_CASIC_DATA:
                if (rx_ptr == CMD_MAX_LEN) {
                    xlog_warn("msg overflow");
                    rx_state = RX_ST_WAIT_HEAD;
                    break;
                } 
				rx_msg.data[rx_ptr++] = rx_byte;
                //xlog_debug("CASIC_DATA [%d] = %02x", rx_ptr - 1, rx_byte);
                if (rx_ptr == 4) {
                    pl_len = rx_msg.data[2] + rx_msg.data[3]*16;
                    //if (gps_debug) xlog_debug("pl_len %d", pl_len);
                }
                //ba ce 04 00 05 0x cm id 00 00 xx xx xx xx 
                //ba ce 08 00 06 00 00 33 00 00 00 c2 01 00 08 f5 07 00
				if (rx_ptr >= pl_len + 10)  {
                    //send to msg_rx_thread
                    rx_msg.q_resv = 10; //magic
                    rx_msg.type = CMD_TYPE_CASIC;
                    rx_msg.len = pl_len + 10;
                    ret = msgsnd(rx_msg_qid, &rx_msg, sizeof(wrap_msg), 0);
                    if (gps_debug) xlog_info("CMD_TYPE_CASIC msgsnd %d, ret %d \n", rx_msg.len, ret);
                    rx_state = RX_ST_WAIT_HEAD;
                }
                break;

            case RX_ST_UBLOX_DATA:
                if (rx_ptr == CMD_MAX_LEN) {
                    xlog_warn("msg overflow");
                    rx_state = RX_ST_WAIT_HEAD;
                    break;
                } 
                rx_msg.data[rx_ptr++] = rx_byte;
                //xlog_debug("CASIC_DATA [%d] = %02x", rx_ptr - 1, rx_byte);
                if (rx_ptr == 6) {
                    pl_len = rx_msg.data[4] + rx_msg.data[5]*16;
                    //if (gps_debug) xlog_debug("pl_len %d", pl_len);
                }
                if (rx_ptr >= pl_len + 8)  {
                    //send to msg_rx_thread
                    rx_msg.q_resv = 10; //magic
                    rx_msg.type = CMD_TYPE_UBLOX;
                    rx_msg.len = pl_len + 10;
                    ret = msgsnd(rx_msg_qid, &rx_msg, sizeof(wrap_msg), 0);
                    if (gps_debug) xlog_info("CMD_TYPE_UBLOX msgsnd %d, ret %d \n", rx_msg.len, ret);
                    rx_state = RX_ST_WAIT_HEAD;
                }
                break;

		}
	}
}

int ublx_gps_checksum(unsigned char *buf, int buf_len)
{
    int i = 0;
    int len = 0;
    unsigned char check_a = 0;
    unsigned char check_b = 0;

    //0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34
    len = buf[5]*256 + buf[4] + 4;
	if (len + 3 >= buf_len) return 0;

    for (i = 2; i < len + 2; i++) {
        check_a = 0xff & (check_a + buf[i]);
        check_b = 0xff & (check_b + check_a);
    }

    buf[len+2] = check_a;
    buf[len+3] = check_b;

    return 0;
}

int send_raw_msg(int cmd_type, char *tx_buff, int tx_len)
{
	int ret, i;

	if (cmd_type == CMD_TYPE_NMEA) 
    {
        char cs_char = 0;
        for (i = 1; i < tx_len - 1; i++) {
            cs_char = cs_char ^ tx_buff[i];
        }
        
        sprintf(tx_buff + tx_len, "%02X\r\n", cs_char);
        ret = write(tty_fd, tx_buff, tx_len + 4);
        if (gps_debug) xlog_info("tx_len %d, write ret %d \n", tx_len + 4, ret);
    }
    else if (cmd_type == CMD_TYPE_UBLOX)
    {
        ublx_gps_checksum(tx_buff, tx_len);
        ret = write(tty_fd, tx_buff, tx_len);
        if (gps_debug) xlog_info("tx_len %d, write ret %d \n", tx_len, ret);
    }
    else //if (cmd_type == CMD_TYPE_CASIC)  
    {
        int cs_int = 0;
        int pl_len = tx_buff[3] * 16 + tx_buff[2];
        int *payload = (int *)(tx_buff + 6);
        
        //Checksum = (ID << 24) + (Class << 16) + Len;
        cs_int = (tx_buff[5] << 24) + (tx_buff[4] << 16) + pl_len;
        for (i = 0; i < pl_len/4; i++) {
            cs_int = cs_int + payload[i];
        }

        tx_buff[tx_len] = cs_int & 0xFF;
        tx_buff[tx_len + 1] = (cs_int >> 8) & 0xFF;
        tx_buff[tx_len + 2] = (cs_int >> 16) & 0xFF;
        tx_buff[tx_len + 3] = (cs_int >> 24) & 0xFF;
        ret = write(tty_fd, tx_buff, tx_len + 4);
        if (gps_debug) xlog_info("tx_len %d, write ret %d \n", tx_len + 4, ret);
    } 
	
	return 0;
}

int send_gnss_cmd(int cmd_id, char *tx_buff, int tx_len, rx_msg_cb msg_cb)
{
	int ret;
    int cmd_type = cmd_id >> 16;

	pthread_mutex_lock(&cmd_mtx);
    pending_cmd = (cmd_type == CMD_TYPE_CASIC) ? cmd_id : 0xFFFF;
    memset(&ack_msg, 0, sizeof(ack_msg));
    vos_sem_clear(&sync_sem);
	ret = send_raw_msg(cmd_type, tx_buff, tx_len);

    if (cmd_type == CMD_TYPE_CASIC) {
    	vos_sem_wait(&sync_sem, 3000);
        if (msg_cb != NULL) msg_cb(&ack_msg);    
    }

	pthread_mutex_unlock(&cmd_mtx);
    
	return ret;
}

int send_ublox_cmd(int cmd_id, char *tx_buff, int tx_len, wrap_msg *ack_data)
{
	int ret;

	pthread_mutex_lock(&cmd_mtx);
    pending_cmd = cmd_id;
    memset(&ack_msg, 0, sizeof(ack_msg));
    vos_sem_clear(&sync_sem);
	ret = send_raw_msg(CMD_TYPE_UBLOX, tx_buff, tx_len);

    vos_sem_wait(&sync_sem, 3000);
    if (ack_data) memcpy(ack_data, &ack_msg, sizeof(wrap_msg));
    if ( (ack_msg.len == 0) || (ack_msg.data[0] != 0xB5) ) {
        pthread_mutex_unlock(&cmd_mtx);
        return VOS_ERR;
    }

	pthread_mutex_unlock(&cmd_mtx);
	return VOS_OK;
}

int atg_gnss_select(int select)
{
    char tx_buff[128];
    int tx_len;
    
    //$PCAS04,mode*hh<CR><LF>
    if (select < 1 || select > 2) select = 3;
    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS04,%d*", select);
    send_gnss_cmd(CMD_PCAS04_GNSS_CFG, tx_buff, tx_len, NULL);
    if (gps_debug) {
        xlog_info("select %d", select);
    }

    return VOS_OK;
}

int atg_gps_lock_state(void)
{
    //Quality 1, status A, pos_type 3
    if ( (gps_info.Quality) 
        && (gps_info.status == 'A')  
         && (gps_info.pos_type == 3) ) {
        return 1;
    }
    if (gps_debug) {
        xlog_info("Quality %d, status %c, pos_type %d", 
                  gps_info.Quality, gps_info.status, gps_info.pos_type);
    }
    return 0;
}

int atg_get_gps_info(struct tm *tmd, int *longitude, int *latitude)
{
    tmd->tm_year    = gps_info.utc_tm.tm_year + 1900;
    tmd->tm_mon     = gps_info.utc_tm.tm_mon + 1;
    tmd->tm_mday    = gps_info.utc_tm.tm_mday;
    tmd->tm_hour    = gps_info.utc_tm.tm_hour;
    tmd->tm_min     = gps_info.utc_tm.tm_min;
    tmd->tm_sec     = gps_info.utc_tm.tm_sec;
    if (gps_debug) {
        xlog_info("INFO: utc time %d-%d-%d %d:%d:%d", 
                   tmd->tm_year, tmd->tm_mon, tmd->tm_mday, 
                   tmd->tm_hour, tmd->tm_min, tmd->tm_sec);
    }

    //Lontiude 11355.979492, Latitude 2231.530273
    *longitude = (int)(gps_info.lontitude*10);
    *latitude  = (int)(gps_info.latitude*10);
}

#endif

#if 1

int cli_cfg_save(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS00*");
    send_gnss_cmd(CMD_PCAS00_SAVE_CFG, tx_buff, tx_len, NULL);
    
    return VOS_OK;
}    

int cli_cfg_loc_intv(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    if (argc < 2) {
        vos_print("%s <interval> //1000, 500, 250, 200ms \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS02,%d*", (int)strtoul(argv[1], 0, 0));
    send_gnss_cmd(CMD_PCAS02_LOC_FREQ, tx_buff, tx_len, NULL);
    
    return VOS_OK;
}    

int cli_nmea_out_enable(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    if (argc < 2) {
        return VOS_E_PARAM;
    }

    //$PCAS03,nGGA,nGLL,nGSA,nGSV,nRMC,nVTG,nZDA,nANT,nDHV,nLPS,res1,res2,nUTC,nGST,res3,res4,res5,nTIM*CS<CR><LF>
    if (strtoul(argv[1], 0, 0)) {
        tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS03,1,0,1,1,1,1,0,0,0,0,,,0,0*");
    } else {
        tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*");
    }

    send_gnss_cmd(CMD_PCAS03_OUT_CFG, tx_buff, tx_len, NULL);
    
    return VOS_OK;
}    

int cli_cfg_gnss_select(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    if (argc < 2) {
        vos_print("%s <mode> //1-gps 2-beidou 3-gps+beidou 6-glonass+beidou \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    //$PCAS04,mode*hh<CR><LF>
    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS04,%d*", (int)strtoul(argv[1], 0, 0));
    send_gnss_cmd(CMD_PCAS04_GNSS_CFG, tx_buff, tx_len, NULL);

    return VOS_OK;
}    

int cli_cfg_get_ver(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    if (argc < 2) {
        vos_print("%s <mode> //0-fw 1-hw 2-mode 3-id \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    //$PCAS06,info*CS<CR><LF>
    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS06,%d*", (int)strtoul(argv[1], 0, 0));
    send_gnss_cmd(CMD_PCAS06_GET_VERSION, tx_buff, tx_len, NULL);

    return VOS_OK;
}    

int cli_cfg_reboot(int argc, char **argv)
{
    char tx_buff[128];
    int tx_len;
    
    if (argc < 2) {
        vos_print("%s <mode> //0-hot 1-warm 2-code 3-ft \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    //$PCAS10,rs*CS<CR><LF>
    tx_len = snprintf(tx_buff, sizeof(tx_buff), "$PCAS10,%d*", (int)strtoul(argv[1], 0, 0));
    send_gnss_cmd(CMD_PCAS10_REBOOT, tx_buff, tx_len, NULL);

    return VOS_OK;
}    

int cli_cfg_gen_cb(wrap_msg *ack_msg)
{
    int i, ptr;
    char line_str[256];
    
    vos_print("MSG(%d): ", ack_msg->len);
    for (i = 0, ptr = 0; i < ack_msg->len; i++) {
        ptr += sprintf(line_str + ptr, "%02x ", ack_msg->data[i]);
        if ( (i + 1)%16 == 0 ) {
            vos_print("%s", line_str);
            ptr = 0;
        }
    } 
    if (ptr) vos_print("%s", line_str);
    vos_print("\r\n");
    return 0;
}

int cli_cfg_ptr_get(int argc, char **argv)
{
    char tx_buff[8] = {0xBA, 0xCE, 0x00, 0x00, 0x01, 0x10};
    int gen_cmd = CMD_CASIC_CFG_PRT;

    tx_buff[4] = gen_cmd >> 8; 
    tx_buff[5] = gen_cmd & 0xFF; 
    send_gnss_cmd(gen_cmd, tx_buff, 6, cli_cfg_gen_cb);
    
    return VOS_OK;
}    

int cli_gen_cmd_get(int argc, char **argv)
{
    char tx_buff[8] = {0xBA, 0xCE, 0x00, 0x00, 0x01, 0x10};
    char cfg_tp[] = {0xBA, 0xCE, 0x10, 0x00, 0x06, 0x03, 
        0x40, 0x42, 0x0F, 0x00, 0xA0, 0x86, 0x01, 0x00, 
        0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 
        0xF2, 0xC8, 0x16, 0x08};
    int gen_cmd;

    if (argc < 2) {
        vos_print("%s <class_id> \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    gen_cmd = (int)strtoul(argv[1], 0, 0);
    if (gen_cmd == 0x0603) {
        send_gnss_cmd(gen_cmd, cfg_tp, sizeof(cfg_tp), cli_cfg_gen_cb);
    } else {
        tx_buff[4] = gen_cmd >> 8; 
        tx_buff[5] = gen_cmd & 0xFF; 
        send_gnss_cmd(gen_cmd, tx_buff, 6, cli_cfg_gen_cb);
    }
    
    return VOS_OK;
}    

int cli_show_gps_info(int argc, char **argv)
{
    vos_print("UTC_TM: %04d-%02d-%02d %02d:%02d:%02d \r\n",
              gps_info.utc_tm.tm_year + 1900, gps_info.utc_tm.tm_mon + 1, gps_info.utc_tm.tm_mday, 
              gps_info.utc_tm.tm_hour, gps_info.utc_tm.tm_min, gps_info.utc_tm.tm_sec);
    vos_print("Lontiude %f, Latitude %f, NumSatUsed %d \r\n",
              gps_info.lontitude, gps_info.latitude, gps_info.NumSatUsed);
    vos_print("Quality %d, status %c, pos_type %d \r\n",
              gps_info.Quality, gps_info.status, gps_info.pos_type);

    vos_print("-----------------------------------------------------------------\r\n");
    vos_print("GSV(%lu) GGA(%lu) GLL(%lu) GSA(%lu) RMC(%lu) ZDA(%lu) \r\n",
              gps_info.GSV_cnt, gps_info.GGA_cnt, gps_info.GLL_cnt, gps_info.GSA_cnt,
              gps_info.RMC_cnt, gps_info.ZDA_cnt);

    for (int i = 0; i < MAX_SAT_NUM; i++) {
        if (sat_info[i].gnss_type) {
            vos_print("[%02d] type %d, svid %2d, elevation %2d, azimuth %3d, cno %2d, timeout %d \r\n",
                      i, sat_info[i].gnss_type, sat_info[i].svid, sat_info[i].elevation,
                      sat_info[i].azimuth, sat_info[i].cno, sat_info[i].timeout);
        }
    }
    
    return VOS_OK;
}    

int cli_gps_debug_ctrl(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("%s <0|1|2> \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    gps_debug = (int)strtoul(argv[1], 0, 0);
    vos_print("gps_debug %d \r\n", gps_debug);
    return VOS_OK;
}

#endif

#if 1

unsigned char ubx_cfg_tp5_set[] = {
0xB5, 0x62, 0x06, 0x31, 0x20, 0x00, 
0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 
0x00, 0x00, 0x00, 0x00, 0xEF, 0x08, 0x00, 0x00, 
0x52, 0x0A};

unsigned char ubx_cfg_tp5_bd[] = {
0xB5, 0x62, 0x06, 0x31, 0x20, 0x00, 
0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 
0x00, 0x00, 0x00, 0x00, 0xEF, 0x09, 0x00, 0x00, 
0x53, 0x0D};

unsigned char ubx_cfg_smgr_set[] = {
0xB5, 0x62, 0x06, 0x62, 0x14, 0x00, 0x00, 0x0F, 0x1E, 0x00, 0x50, 0x00, 0x00, 0x00, 0xFA, 0x00,
0xD0, 0x07, 0x0F, 0x00, 0x10, 0x27, 0xCA, 0x70, 0x00, 0x00, 0x4A, 0x79};

int ubx_get_pvt_status()
{
    char tx_buff[8] = {0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19};
    wrap_msg ack_data;
    int ret, fix_type = 0;

    ret = send_ublox_cmd(0x0107, tx_buff, sizeof(tx_buff), &ack_data);
    if (ret != VOS_OK) return 0;

	fix_type = ack_data.data[UBX_PL_START + 20];
    return fix_type;
}

int ubx_gnss_select(int select)
{
    unsigned char ubx_cfg_gnss_set[] = {
    0xB5, 0x62, 0x06, 0x3E, 0x2C, 0x00, 
    0x00, 0x00, 0x20, 0x05, 
    0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,         //GPS
    0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01,         //SBAS
    0x03, 0x08, 0x10, 0x00, 0x00, 0x00, 0x01, 0x01,         //BEIDOU
    0x05, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,         //QZSS
    0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01,         //GLONASS
    0xFE, 0x21};
    char *pTemp = &ubx_cfg_gnss_set[0];

    if (gps_debug) {
        xlog_info("select %d", select);
    }
	if (select < 2) {
		pTemp[UBX_PL_START + 8] = 0x1;	//GPS
		pTemp[UBX_PL_START + 32] = 0x1;	//QZSS
	} else {
		pTemp[UBX_PL_START + 8] = 0x0;
		pTemp[UBX_PL_START + 32] = 0x0;
	}
	
	if (select == 0 || select == 2) {
		pTemp[UBX_PL_START + 24] = 0x1;
	} else {
		pTemp[UBX_PL_START + 24] = 0x0;
	}
	
	if (select == 0) {
		pTemp[UBX_PL_START + 40] = 0x1;
	} else {
		pTemp[UBX_PL_START + 40] = 0x0;
	}

    //ack b5 62 05 01 02 00 06 3e 4c 75 30 31
	send_raw_msg(CMD_TYPE_UBLOX, ubx_cfg_gnss_set, sizeof(ubx_cfg_gnss_set));
    usleep(100*1000);

    if (select < 2) {
        send_raw_msg(CMD_TYPE_UBLOX, ubx_cfg_tp5_set, sizeof(ubx_cfg_tp5_set));
    } else {
        send_raw_msg(CMD_TYPE_UBLOX, ubx_cfg_tp5_bd, sizeof(ubx_cfg_tp5_bd));
    }

	/* change ubx_cfg_smgr_set to 1 - Non-coherent pulses. 
	In this mode the receiver will correct time phase offsets as quickly as
	allowed by the specified maximum slew rate, in which case there may not be the expected number of
	GNSS oscillator cycles between time pulses. 
	*/
	send_raw_msg(CMD_TYPE_UBLOX, ubx_cfg_smgr_set, sizeof(ubx_cfg_smgr_set));

	pthread_mutex_unlock(&cmd_mtx);
    return 0;
}

int ubx_get_gps_info(struct tm *tmd, int *longitude, int *latitude)
{
    char tx_buff[8] = {0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19};
    wrap_msg ack_data;
    int ret, value;

    ret = send_ublox_cmd(0x0107, tx_buff, sizeof(tx_buff), &ack_data);
    if (ret != VOS_OK) return ret;

    value = ack_data.data[UBX_PL_START + 5];
    value = (value << 8) | ack_data.data[UBX_PL_START + 4];
    tmd->tm_year    = value;
    tmd->tm_mon     = ack_data.data[UBX_PL_START + 6];
    tmd->tm_mday    = ack_data.data[UBX_PL_START + 7];
    tmd->tm_hour    = ack_data.data[UBX_PL_START + 8];
    tmd->tm_min     = ack_data.data[UBX_PL_START + 9];
    tmd->tm_sec     = ack_data.data[UBX_PL_START + 10];
    if (gps_debug) {
        xlog_info("INFO: utc time %d-%d-%d %d:%d:%d", 
                   tmd->tm_year, tmd->tm_mon, tmd->tm_mday, 
                   tmd->tm_hour, tmd->tm_min, tmd->tm_sec);
    }

    value = ack_data.data[UBX_PL_START + 27];
    value = (value << 8) | ack_data.data[UBX_PL_START + 26];
    value = (value << 8) | ack_data.data[UBX_PL_START +  25];
    value = (value << 8) | ack_data.data[UBX_PL_START + 24];
    *longitude = value/1E4;
    value = ack_data.data[UBX_PL_START + 31];
    value = (value << 8) | ack_data.data[UBX_PL_START + 30];
    value = (value << 8) | ack_data.data[UBX_PL_START +  29];
    value = (value << 8) | ack_data.data[UBX_PL_START + 28];
    *latitude = value/1E4;

    return 0;
}

int ubx_gps_lock_state(void) //ublx_gps_TIM_TP_show
{
    char tx_buff[8] = {0xB5, 0x62, 0x0D, 0x01, 0x00, 0x0, 0x0E, 0x37};
    unsigned long TOWu4 = 0;
    unsigned short weeku2 = 0;
    unsigned long timeTows = 0;
    int com512 = 0;
    int gps_sfn = 0;
    unsigned int gps_tod = 0;
    wrap_msg ack_data;
    int ret;

    if (ubx_get_pvt_status() != 3) {
        return 0;
    }

    ret = send_ublox_cmd(0x0d01, tx_buff, sizeof(tx_buff), &ack_data);
    if (ret != VOS_OK) return 0;

    //b5 62 0d 01 10 00 70 4a 4d 11 00 00 00 00 00 00 00 00 e8 08 02 00 28 5c 30 32
    if (ack_data.data[2] != 0x0D || ack_data.data[3] != 0x01) {
        return 0;
    }

    TOWu4 = ack_data.data[UBX_PL_START + 3];
    TOWu4 = (TOWu4 << 8) | ack_data.data[UBX_PL_START + 2];
    TOWu4 = (TOWu4 << 8) | ack_data.data[UBX_PL_START +  1];
    TOWu4 = (TOWu4 << 8) | ack_data.data[UBX_PL_START ];

    weeku2 = ack_data.data[UBX_PL_START + 13];
    weeku2 = (weeku2 << 8) | ack_data.data[UBX_PL_START + 12];
	
    TOWu4 = TOWu4 + 1;  //  +1ms
    timeTows = TOWu4 / 1000; //turn to integer seconds
    com512 = ((weeku2 * 7) % 2 != 0 )? 512 : 0;
	
    /*com512: 0 or 512,   100: fpga run when next second */
    gps_sfn = ((timeTows * 100 + com512) % 1024);

    /* 1970-1-1--> 1980-1-6 : 315964800*/
    gps_tod = (weeku2 * 7 * 86400) + 315964800 + (TOWu4/1000);

    if (gps_debug) xlog_info("tpsfn: 0x%08x tod: 0x%08x", gps_sfn, gps_tod);
    return 1;
}

int cli_ublox_mon_ver_get(int argc, char **argv)
{
    char tx_buff[8] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34};
    wrap_msg ack_data;
    int payload_len, ret;
	char *swVer;
	char *hwVer;

    ret = send_ublox_cmd(0x0A04, tx_buff, sizeof(tx_buff), &ack_data);
    if (ret != VOS_OK) return ret;

    payload_len = ack_data.data[UBX_PL_START - 1];
    payload_len = (payload_len << 8) + ack_data.data[UBX_PL_START - 2];
    swVer = &ack_data.data[UBX_PL_START];
    hwVer = &ack_data.data[UBX_PL_START + 30];
    vos_print("swVersion %s, hwVersion %s \n", swVer, hwVer);
    
    for (int i = 0; i < (payload_len - 40)/30; i++ ) {
        swVer = &ack_data.data[UBX_PL_START + 40 + i*30];
        if (*swVer == 0) break;
        vos_print("%s \n", swVer);
    }

    return VOS_OK;
}    

int cli_ublox_sat_show(int argc, char **argv)
{
    char tx_buff[8] = {0xB5, 0x62, 0x01, 0x35, 0x00, 0x00, 0x36, 0xA3};
    wrap_msg ack_data;
	int ret, payload_len, numSvs, i;
	int gnssId, svId, cno, elev, azim, flags;
	short prRes;

    ret = send_ublox_cmd(0x0135, tx_buff, sizeof(tx_buff), &ack_data);
    if (ret != VOS_OK) return ret;

	payload_len = ack_data.data[UBX_PL_START - 1];
	payload_len = (payload_len << 8) + ack_data.data[UBX_PL_START - 2];
	numSvs = ack_data.data[UBX_PL_START + 5];
	if ((numSvs < 1) || (payload_len < numSvs*12 + 8)) {
		vos_print("read error %d %d\n", numSvs, payload_len);
		return VOS_ERR;
	}
	
	for ( i = 0; i < numSvs; i++ ) {
		gnssId 	= ack_data.data[UBX_PL_START + 12*i + 8];
		svId 	= ack_data.data[UBX_PL_START + 12*i + 9];
		cno 	= ack_data.data[UBX_PL_START + 12*i + 10];
		elev 	= ack_data.data[UBX_PL_START + 12*i + 11];
		azim 	= ack_data.data[UBX_PL_START + 12*i + 13];
		azim 	= (azim << 8) | ack_data.data[UBX_PL_START + 12*i + 12];
		prRes 	= ack_data.data[UBX_PL_START + 12*i + 15];
		prRes 	= (prRes << 8) | ack_data.data[UBX_PL_START + 12*i + 14];
        flags 	= ack_data.data[UBX_PL_START + 12*i + 19];
        flags 	= (flags << 8) | ack_data.data[UBX_PL_START + 12*i + 18];
        flags 	= (flags << 8) | ack_data.data[UBX_PL_START + 12*i + 17];
        flags 	= (flags << 8) | ack_data.data[UBX_PL_START + 12*i + 16];

		if ((gnssId > 10) || (svId == 0) || (flags == 0)) continue;
		vos_print("gnssId=%d svId=%d cno=%d elev=%d azim=%d prRes=%d flags=0x%x\n", 
				 gnssId, svId, cno, elev, azim, prRes, flags);
	}

    return VOS_OK;
}    

int cli_ublox_gnss_select(int argc, char **argv)
{
    int select, ret;

    if (argc < 2) {
        vos_print("%s <0-gps&bd, 1-gps, 2-bd> \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    select = (int)strtoul(argv[1], 0, 0);
    ret = ubx_gnss_select(select);
    vos_print("ubx_gnss_select ret %d \r\n", ret);
    
    return VOS_OK;
}    

int cli_ublox_gen_cmd(int argc, char **argv)
{
    char tx_buff[8] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34};
    wrap_msg ack_data;
    int gen_cmd;

    if (argc < 2) {
        vos_print("%s <class_id> //0 or 0x0a04 \r\n", argv[0]);
        return VOS_E_PARAM;
    }

    gen_cmd = (int)strtoul(argv[1], 0, 0);
    if (gen_cmd > 0) {
        tx_buff[2] = gen_cmd >> 8; 
        tx_buff[3] = gen_cmd & 0xFF; 
        send_ublox_cmd(gen_cmd, tx_buff, sizeof(tx_buff), &ack_data);
        cli_cfg_gen_cb(&ack_data);
        return VOS_OK;
    }

    struct tm tmd; 
    int longitude, latitude;
    vos_print("fix_type: %d \r\n", ubx_get_pvt_status());
    vos_print("lock_stat: %d \r\n", ubx_gps_lock_state());
    ubx_get_gps_info(&tmd, &longitude, &latitude);
    vos_print("tm %d-%d-%d %02d:%02d:%02d, pos %d %d \r\n",
              tmd.tm_year, tmd.tm_mon, tmd.tm_mday,
              tmd.tm_hour, tmd.tm_min, tmd.tm_sec,
              longitude, latitude);
    
    return VOS_OK;
}    

#endif

int gnss_mng_init(int type, char *tty_dev)
{
	int ret;
    pthread_t unused_tid;  

    module_type = type;
    memset(&gps_info, 0, sizeof(gps_info));
	tty_fd = open(tty_dev, O_RDWR | O_NOCTTY);
	if (tty_fd < 0) {
		xlog_warn("failed to open %s \n", tty_dev);
		return 0;
	}
	tty_set_raw(tty_fd, type);
    xlog_info("tty_fd %d", tty_fd);

    rx_msg_qid = msgget(IPC_PRIVATE, 0666);
    if (rx_msg_qid == -1) {
        xlog_warn("msgget error");
        return -1;
    }

	sem_init(&sync_sem, 0, 0); 
	pthread_mutex_init(&cmd_mtx, NULL); 

    ret = pthread_create(&unused_tid, NULL, (void *)tty_rx_thread, NULL);  
    if (ret != 0)  {  
        xlog_warn("Create pthread error!\n");  
        return -1;  
    }      

    ret = pthread_create(&unused_tid, NULL, (void *)msg_rx_thread, NULL);  
    if (ret != 0)  {  
        xlog_warn("Create pthread error!\n");  
        return -1;  
    }

    #ifndef DEMO_APP
    cli_cmd_reg("gps_info",         "show gps info",            &cli_show_gps_info);
    cli_cmd_reg("gps_debug",        "gps debug ctrl",            &cli_gps_debug_ctrl);
    if (module_type) {
        cli_cmd_reg("gnss_select",      "gnss select",              &cli_cfg_gnss_select);
        cli_cmd_reg("gps_cmd",          "gps cmd test",             &cli_gen_cmd_get);
        cli_cmd_reg("gps_reboot",       "gps reboot",               &cli_cfg_reboot);
    } else {
        cli_cmd_reg("gnss_select",      "gnss select",              &cli_ublox_gnss_select);
        cli_cmd_reg("gps_cmd",          "gps cmd test",             &cli_ublox_gen_cmd);
        cli_cmd_reg("gnss_ver",         "gnss version",             &cli_ublox_mon_ver_get);
        cli_cmd_reg("gnss_sat",         "gnss sat show",            &cli_ublox_sat_show);
    }
    #endif

	return 0;
}

#ifdef DEMO_APP

/*
aarch64-linux-gnu-gcc -o gnss_mng.bin gnss_mng.c -DDEMO_APP -lpthread
./gnss_mng.bin /dev/ttyS8 1
*/
int main(int argc, char **argv)
{
    int new_argc, type, log_level;
    char *new_argv[32];
    char string[512];

    if (argc < 3) {
        vos_print("usage: %s </dev/ttyx> <type> [<log_level>] \n", argv[0]);
        return 0;
    }

    gps_debug = 1;
    type = (argc > 2) ? atoi(argv[2]) : 1;
    log_level = (argc > 3) ? atoi(argv[3]) : 7;
    xlog_init(log_level);
    gnss_mng_init(type, argv[1]);
    while (1) {
        vos_print("-----------------------------------------------------------------\r\n");
        vos_print("PCAS00                   -- PCAS00 cfg save \r\n");
        vos_print("PCAS02 <interval>        -- PCAS02 cfg fixIntv \r\n");
        vos_print("PCAS03 <0|1>             -- PCAS03 NMEA out enable \r\n");
        vos_print("PCAS06 <mode>            -- PCAS06 get verion \r\n");
        vos_print("PCAS10 <mode>            -- PCAS06 reboot \r\n");
        vos_print("cfg_ptr_get              -- CFG-PRT GET \r\n");
        vos_print("gen_cmd                  -- CASIC-CMD \r\n");
        vos_print("gps_info                 -- GPS INFO Summary \r\n");
        vos_print("gps_debug                -- GPS debug ctrl \r\n");
        vos_print("gnss_sel                 -- gnss select \r\n");
        
        vos_print("\r\ninput cmd: ");
        fgets(string, sizeof(string), stdin);

        //cli_param_format
        new_argc = cli_param_format(string, new_argv, 32);
        //for (int i = 0; i < new_argc; i++) 
            //vos_print("[%d]=%s \r\n", i, new_argv[i]);
        
        if (memcmp(new_argv[0], "exit", 4) == 0) 
            break;
        else if (memcmp(new_argv[0], "PCAS00", 6) == 0) 
            cli_cfg_save(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "PCAS02", 6) == 0) 
            cli_cfg_loc_intv(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "PCAS03", 6) == 0) 
            cli_nmea_out_enable(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "PCAS06", 6) == 0) 
            cli_cfg_get_ver(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "PCAS10", 6) == 0) 
            cli_cfg_reboot(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "cfg_ptr_get", 11) == 0) 
            cli_cfg_ptr_get(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "gen_cmd", 7) == 0) 
            cli_gen_cmd_get(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "gps_info", 7) == 0) 
            cli_show_gps_info(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "ublox_cmd", 9) == 0) 
            cli_ublox_gen_cmd(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "mon_ver", 7) == 0) 
            cli_ublox_mon_ver_get(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "sat_show", 8) == 0) 
            cli_ublox_sat_show(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "gnss_sel", 8) == 0) 
            cli_cfg_gnss_select(new_argc, &new_argv[0]);
        else if (memcmp(new_argv[0], "gps_debug", 8) == 0) 
            cli_gps_debug_ctrl(new_argc, &new_argv[0]);
        else 
            vos_print("unknown cmd %s \r\n", new_argv[0]);
    }

    return VOS_OK;
}

#endif

