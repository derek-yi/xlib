#define _GNU_SOURCE 
#include "vos.h"

#if 1

int sys_read_pipe(char *cmd_str, char *buff, int buf_len)
{
	FILE *fp;

    if (!cmd_str) return VOS_ERR;
    if (!buff) return VOS_ERR;
    
	fp = popen(cmd_str, "r");
    if (fp == NULL) {
        return VOS_ERR;
    }
    
    memset(buff, 0, buf_len);
	fgets(buff, buf_len, fp);
	pclose(fp);
    return VOS_OK;
}

int sys_node_readstr(char *node_str, char *rd_buf, int buf_len)
{
	FILE *fp;

	if (rd_buf == NULL) return VOS_ERR;

	fp = fopen(node_str, "r");
	if (fp == NULL) {
		return VOS_ERR;
	}
	
	memset(rd_buf, 0, buf_len);
	fgets(rd_buf, buf_len, fp);
	fclose(fp);

    for (int i = 0; i < strlen(rd_buf); i++) {
        if ( (rd_buf[i] == '\r') || (rd_buf[i] == '\n') ) {
            rd_buf[i] = 0; break;
        }
    }
	return VOS_OK;
}

int sys_node_read(char *node_str, int *value)
{
    char rd_buf[256];

    if ( sys_node_readstr(node_str, rd_buf, sizeof(rd_buf)) != VOS_OK)
		return VOS_ERR;
	
    if (value) {
        *value = (int)strtoul(rd_buf, 0, 0);
    }
    
    return VOS_OK;
}

int sys_node_writestr(char *node_str, char *wr_buf)
{
	FILE *fp;

	if (wr_buf == NULL) return VOS_ERR;

	fp = fopen(node_str, "w");
	if (fp == NULL) {
		return VOS_ERR;
	}
	
	fwrite(wr_buf, strlen(wr_buf), 1, fp);
    fclose(fp);
    return VOS_OK;
}

int sys_node_write(char *node_str, int value)
{
    char val_str[64];

    snprintf(val_str, sizeof(val_str), "0x%x", value);
    return sys_node_writestr(node_str, val_str);
}

#endif

#if 1

/*
union sigval {
    int sival_int;
    void *sival_ptr;
};
*/
typedef void (* lib_callback)(union sigval);

int vos_create_timer(timer_t *ret_tid, int interval, int repeat, timer_cb callback, void *param)
{
	timer_t timerid;
	struct sigevent evp;
	struct itimerspec it;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = param; 
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = (lib_callback)callback;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
        *ret_tid = 0; //0 as gift 
		return -1;
	}
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长
	it.it_value.tv_sec = interval/1000;
	it.it_value.tv_nsec = (interval%1000)*1000*1000;
	if ( repeat ) {
		it.it_interval.tv_sec  = it.it_value.tv_sec;
		it.it_interval.tv_nsec = it.it_value.tv_nsec;
	} else {
		it.it_interval.tv_sec  = 0;
		it.it_interval.tv_nsec = 0;
	}
    
    timer_settime(timerid, 0, &it, NULL);  //not care result
    *ret_tid = timerid;
    return 0;
}

int vos_delete_timer(timer_t timerid)
{
    if (timerid == 0) return -1; //0 cause segment fault
	return timer_delete(timerid);
}

void vos_msleep(uint32 milliseconds) 
{
    struct timespec ts = {
        milliseconds / 1000,
        (milliseconds % 1000) * 1000000
    };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

//https://blog.csdn.net/Primeprime/article/details/60954203
int vos_run_cmd(const char *format, ...)
{
	va_list args;
	char cmd_str[1024];
    int status;
    
    va_start(args, format);
    vsnprintf(cmd_str, 1024, format, args);
    va_end(args);
    
    status = system(cmd_str);
    if (status < 0)
    {
        return status;
    }
     
    if (WIFEXITED(status))
    {
        //printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
        return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        //printf("abnormal termination,signal number =%d\n", WTERMSIG(status)); //如果cmdstring被信号中断，取得信号值
        return VOS_ERR;
    }
    else if (WIFSTOPPED(status))
    {
        //printf("process stopped, signal number =%d\n", WSTOPSIG(status)); //如果cmdstring被信号暂停执行，取得信号值
        return VOS_ERR;
    }

    return VOS_OK;
}

int vos_sem_wait(void *sem_id, uint32 msecs)
{
	struct timespec ts;
    sem_t *sem = (sem_t *)sem_id;
	uint32 add = 0;
	uint32 secs = msecs/1000;
    
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

void fmt_time_str(char *time_str, int max_len)
{
    struct tm *tp;
    time_t t = time(NULL);
    tp = localtime(&t);
     
    if (!time_str) return ;
    
    snprintf(time_str, max_len, "%02d%02d_%02d_%02d_%02d", 
            tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

int utc_tm_convert(struct tm *utc_tm, struct tm *bj_tm)
{
    struct tm time;
    time_t timestamp;
    
    if (utc_tm == NULL || bj_tm == NULL) return -1;
    time = *utc_tm;
    if (time.tm_year > 1900) {
        time.tm_year -= 1900; //refer to struct tm
        time.tm_mon -= 1; 
    }

    timestamp = mktime(&time);
    timestamp += 8*60*60;
    localtime_r(&timestamp, &time);
    //time = localtime(&timestamp);
    //time = gmtime(&timestamp);

    time.tm_year += 1900;
    time.tm_mon += 1; 
    *bj_tm = time;

	return 0;
}

int vos_file_exist(char *file_path)
{
    if (access(file_path, F_OK) == 0)
        return TRUE;
    
    return FALSE;
}

long vos_file_size(char *file_path)
{
    struct stat info;
    
    if (stat(file_path, &info) == -1) {
        return 0;
    }

    return info.st_size;
}

int vos_bind_to_cpu(int cpu_id)
{
	cpu_set_t mask; 

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}

int vos_set_self_name(char *thread_name)
{
    pthread_setname_np(pthread_self(), thread_name);
    return VOS_OK;
}

long vos_get_ts(void)
{
    return (long)time(NULL);
}

void get_cpu_occupy(CPU_OCCUPY *cpu_occupy)
{
    FILE *fp;
    char buff[256];

    fp = fopen("/proc/stat", "r");
    if (fp != NULL) {
        fgets(buff, sizeof(buff), fp);
        sscanf(buff, "%s %lu %lu %lu %lu",
               cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice, 
               &cpu_occupy->system, &cpu_occupy->idle);
        fclose(fp);
    }
}

int vos_get_cpu_occupy(CPU_OCCUPY *o, CPU_OCCUPY *n)
{
    int cpu_use = 0;
    long od, nd;
    long id, sd;

    od = (unsigned long)(o->user + o->nice + o->system + o->idle);
    nd = (unsigned long)(n->user + n->nice + n->system + n->idle);
    id = (unsigned long)(n->user - o->user);
    sd = (unsigned long)(n->system - o->system);
    if ((nd-od) != 0) cpu_use = (int)((sd+id)*100)/(nd-od);

    return cpu_use;
}

//https://www.cnblogs.com/frytea/p/13411364.html
static int get_mem_occupy(MEM_OCCUPY *o)
{
    int i = 0;
    long value;
    char name[256];
    char line[256];

    FILE* fp = fopen("/proc/meminfo", "r");
    if (NULL == fp) {
        return 1;
    }

    while (fgets(line, sizeof(line) - 1, fp)) {
        if (sscanf(line, "%s %lu", name, &value) != 2) {
            continue;
        }
        if (0 == strcmp(name, "MemTotal:")) {
            ++i;
            o->total = value;
        } else if (0 == strcmp(name, "MemFree:")) {
            ++i;
            o->free = value;
        } else if (0 == strcmp(name, "MemAvailable:")) {
            ++i;
            o->available = value;
        } else if (0 == strcmp(name, "Buffers:")) {
            ++i;
            o->buffers = value;
        } else if (0 == strcmp(name, "Cached:")) {
            ++i;
            o->cached = value;
        }
        if (i == 5) {
            break;
        }
    }

    fclose(fp);
    return 0;
}

int vos_get_mem_occupy(void)
{
    MEM_OCCUPY info;

    if (get_mem_occupy(&info) != 0) return 0;
    if (info.total == 0) return 0;
    return (100 * (info.total - info.available) / info.total);
}

int get_local_ip(char *if_name)
{
    int inet_sock;  
    struct ifreq ifr;  

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, if_name);  
    if ( ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0 ) {
		close(inet_sock);
        return -1;
    }

	close(inet_sock);
    return (int)((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

int get_local_mac(char *if_name, char *mac_addr)
{
    int sockfd;  
    struct ifreq ifr;  

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, if_name);  
	
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0) {
		memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);
		//printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	close(sockfd);
    return VOS_OK;
}

int vos_send_udp_pkt(char *dst_ipaddr, int dst_port, void *tx_buff, int len) 
{
    struct sockaddr_in srv_addr;
    int tmp_sock, ret;
    
    tmp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (tmp_sock < 0) {
        return -1;
    }

    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(dst_port);
    if (dst_ipaddr == NULL) srv_addr.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDR);
    else srv_addr.sin_addr.s_addr = inet_addr(dst_ipaddr);
    ret = sendto(tmp_sock, tx_buff, len, 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret <= 0) {
        close(tmp_sock);
        return -1;
    }

    close(tmp_sock);
    return 0;
}

#endif

#if 1

#include <termios.h>

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
    } else if (baud == 2) {
    	cfsetispeed(&newtio, B19200);
	    cfsetospeed(&newtio, B19200);
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

#endif

#if 1

unsigned int hweight32(unsigned int w)
{
    unsigned int res = w - ((w >> 1) & 0x55555555);
    res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
    res = (res + (res >> 4)) & 0x0F0F0F0F;
    res = res + (res >> 8);
    return (res + (res >> 16)) & 0x000000FF;
}

uint32_t hweight8(uint32_t w)
{
    return hweight32((unsigned int)w);
}

short mk_num16(char high, char low) 
{
	short temp = high;
	temp = (temp << 8) + low;
	return temp;
}

int mk_num32(char b0, char b1, char b2, char b3) 
{
	int temp = b0;
	temp = (temp << 8) + b1;
	temp = (temp << 8) + b2;
	temp = (temp << 8) + b3;
	return temp;
}

int mk_boundary(int frame, int slot, int symbol)
{
   return (symbol << 20) + ((slot&0xFF) << 12) + (frame & 0xFFF);
}

int list_max_index2(uint64_t *list, int size)
{
	uint64_t max_value = 0;
	uint32 i, max_index = 0;

	for (i = 0; i < size; i++) {
		if (list[i] > max_value) {
			max_index = i;
			max_value = list[i];
		}
	}

	return max_index;
}

#endif

#define INCLUDE_DEV_MEM
#ifdef INCLUDE_DEV_MEM

#define MAP_SIZE                4096UL
#define MAP_MASK                (MAP_SIZE - 1)
//#define FPGA_BASE_ADDR           0x900000000

uint32 devmem_read(uint64_t mem_addr) 
{
    void *map_base, *virt_addr;
    uint32 read_result;
    int memdev_fd = -1;
    //off_t target = (off_t)((mem_addr & 0x0FFFFFFF) | FPGA_BASE_ADDR);

    if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 ) {
        x_perror("open");
        return 0;
    }

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, mem_addr & ~MAP_MASK);
    if (map_base == (void *) -1) {
        x_perror("mmap");
        close(memdev_fd);
        return 101;
    }
    
    virt_addr = map_base + (mem_addr & MAP_MASK);
	read_result = *((volatile uint32 *) virt_addr);
    if (munmap(map_base, MAP_SIZE) == -1) {
        x_perror("munmap");
    }

    close(memdev_fd);
    return read_result;
}

uint32 devmem_write(uint64_t mem_addr, uint32 writeval) 
{
    void *map_base, *virt_addr;
    //off_t target = (off_t)((mem_addr & 0x0FFFFFFF) | FPGA_BASE_ADDR);
    int memdev_fd = -1;

    if ( (memdev_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1 ) {
        x_perror("open");
        return 0;
    }

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memdev_fd, mem_addr & ~MAP_MASK);
    if (map_base == (void *) -1) {
        x_perror("mmap");
        close(memdev_fd);
        return 0;
    }
    
    virt_addr = map_base + (mem_addr & MAP_MASK);
	*((volatile uint32 *) virt_addr) = writeval;
    if (munmap(map_base, MAP_SIZE) == -1)  {
        x_perror("munmap");
    }

    close(memdev_fd);
    return 0;
}

#endif

#if 1

/**
https://www.cnblogs.com/skullboyer/p/8342167.html
*/
void InvertUint8(unsigned char *DesBuf, unsigned char *SrcBuf)
{
    int i;
    unsigned char temp = 0;

    for(i = 0; i < 8; i++)
    {
        if(SrcBuf[0] & (1 << i))
        {
            temp |= 1<<(7-i);
        }
    }
    DesBuf[0] = temp;
}

void InvertUint16(unsigned short *DesBuf, unsigned short *SrcBuf)
{
    int i;
    unsigned short temp = 0;

    for(i = 0; i < 16; i++)
    {
        if(SrcBuf[0] & (1 << i))
        {
            temp |= 1<<(15 - i);
        }
    }
    DesBuf[0] = temp;
}

unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0x0000;
    unsigned short wCPoly = 0x1021;
    unsigned char wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        InvertUint8(&wChar, &wChar);
        wCRCin ^= (wChar << 8);

        for(int i = 0; i < 8; i++)
        {
            if(wCRCin & 0x8000)
            {
                wCRCin = (wCRCin << 1) ^ wCPoly;
            }
            else
            {
                wCRCin = wCRCin << 1;
            }
        }
    }
	
    InvertUint16(&wCRCin, &wCRCin);
    return (wCRCin) ;
}

unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0xFFFF;
    unsigned short wCPoly = 0x1021;
    unsigned char wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        wCRCin ^= (wChar << 8);

        for(int i = 0; i < 8; i++)
        {
            if(wCRCin & 0x8000)
            {
                wCRCin = (wCRCin << 1) ^ wCPoly;
            }
            else
            {
                wCRCin = wCRCin << 1;
            }
        }
    }
    return (wCRCin) ;
}

int msg_check_crc16(char *msg, int msg_len, unsigned short crc16)
{
	if ( crc16 != CRC16_CCITT(msg, msg_len) ) {
		return 0; //todo
	}

	return 0;
}

int msg_calc_crc16(char *msg, int msg_len)
{
	unsigned short crc16;

	crc16 = CRC16_CCITT(msg, msg_len);
	msg[msg_len] = crc16 & 0xFF;
	msg[msg_len + 1] = crc16 >> 8;
	
	return 0;
}

static unsigned short int fcstab[256]=
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

unsigned short vos_crc16(unsigned char *d, unsigned short len, unsigned short initcrc)
{
    unsigned short i;
    unsigned short crc;

    crc = initcrc;
    for (i=0; i<len; i++) {
        crc = (crc<<8)^fcstab[(crc>>8)^*d++];
    }
    return crc;
}

#endif

#if 1

char base46_map[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                     'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                     'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};


char* base64_encode(char* plain) 
{
    char counts = 0;
    char buffer[3];
    char* cipher = malloc(strlen(plain) * 4 / 3 + 4);
    int i = 0, c = 0;

    for(i = 0; plain[i] != '\0'; i++) {
        buffer[counts++] = plain[i];
        if(counts == 3) {
            cipher[c++] = base46_map[buffer[0] >> 2];
            cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) + (buffer[1] >> 4)];
            cipher[c++] = base46_map[((buffer[1] & 0x0f) << 2) + (buffer[2] >> 6)];
            cipher[c++] = base46_map[buffer[2] & 0x3f];
            counts = 0;
        }
    }

    if(counts > 0) {
        cipher[c++] = base46_map[buffer[0] >> 2];
        if(counts == 1) {
            cipher[c++] = base46_map[(buffer[0] & 0x03) << 4];
            cipher[c++] = '=';
        } else {                      // if counts == 2
            cipher[c++] = base46_map[((buffer[0] & 0x03) << 4) + (buffer[1] >> 4)];
            cipher[c++] = base46_map[(buffer[1] & 0x0f) << 2];
        }
        cipher[c++] = '=';
    }

    cipher[c] = '\0';   /* string padding character */
    return cipher;
}

char* base64_decode(char* cipher) 
{
    char counts = 0;
    char buffer[4];
    char* plain = malloc(strlen(cipher) * 3 / 4);
    int i = 0, p = 0;

    for(i = 0; cipher[i] != '\0'; i++) {
        char k;
        for(k = 0 ; k < 64 && base46_map[k] != cipher[i]; k++);
        buffer[counts++] = k;
        if(counts == 4) {
            plain[p++] = (buffer[0] << 2) + (buffer[1] >> 4);
            if(buffer[2] != 64)
                plain[p++] = (buffer[1] << 4) + (buffer[2] >> 2);
            if(buffer[3] != 64)
                plain[p++] = (buffer[2] << 6) + buffer[3];
            counts = 0;
        }
    }

    plain[p] = '\0';    /* string padding character */
    return plain;
}

//https://github.com/ogay/rc4
static const uint8 rc4_table[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff};

#define SWAP(x, y) \
{                  \
    tmp = x;       \
    x = y;         \
    y = tmp;       \
}

#define RC4_CRYPT()                                             \
{                                                               \
    t += s[(uint8) i];                                          \
    SWAP(s[(uint8) i], s[t]);                                   \
    new_dst[i] = new_src[i] ^ s[(uint8) (s[(uint8) i] + s[t])]; \
}

void rc4_ks(rc4_ctx *ctx, const uint8 *key, uint32 key_len)
{
    uint32 i;
    uint8 *s;
    uint8 t, tmp;

    t = 0;
    s = ctx->se;
    //assert(key_len > 0 && key_len <= 256);

    ctx->pose = 1;
    ctx->posd = 1;
    ctx->te = 0;
    ctx->td = 0;
    memcpy(s, rc4_table, 256);
    for (i = 0; i < 256; i++) {
        t += s[i] + key[i % key_len];
        SWAP(s[i], s[t]);
    }

    memcpy(ctx->sd, s, 256);
}

void rc4_encrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len)
{
    uint32 i;
    uint32 pos;
    const uint8 *new_src;
    uint8 *s, *new_dst;
    uint8 t, tmp;

    pos = ctx->pose;
    s = ctx->se;
    t = ctx->te;

    new_src = src - pos;
    new_dst = dst - pos;
    for (i = pos; i < len + pos; i++) {
        RC4_CRYPT();
    }

    ctx->pose = i;
    ctx->te = t;
}

void rc4_decrypt(rc4_ctx *ctx, const uint8 *src, uint8 *dst, uint32 len)
{
    uint32 i;
    uint32 pos;
    const uint8 *new_src;
    uint8 *s, *new_dst;
    uint8 t, tmp;

    pos = ctx->posd;
    s = ctx->sd;
    t = ctx->td;

    new_src = src - pos;
    new_dst = dst - pos;
    for (i = pos; i < len + pos; i++) {
        RC4_CRYPT();
    }

    ctx->posd = i;
    ctx->td = t;
}

//https://www.toolhelper.cn/SymmetricEncryption/RC4
int rc4_main()
{
	char *key = "0123456789abcdefg";
    rc4_ctx ctx;
	char *pt = "{\"lic_enable\":1, \"expired_day\":\"20250101\", \"credit_max\":1000}";
    uint8 dst[512];
	uint8 decode[512];
	char *bs_str, *new_str;
	int i;

    rc4_ks(&ctx, key, strlen(key));
    rc4_encrypt(&ctx, pt, dst, strlen(pt));
	
	printf("rc4_encrypt: ");
	for (i = 0; i < 128; i++) {
		if (dst[i] == 0) break;
		printf("0x%02x ", dst[i]);
	}
	printf("\n");
	printf("strlen(pt) %d %d\n", (int)strlen(pt), i);
	
	bs_str = base64_encode(dst);
	printf("bs_str:%s \n", bs_str);
	
	new_str = base64_decode(bs_str);
	for (i = 0; i < 128; i++) {
		if (new_str[i] == 0) break;
	}
	
	//rc4_decrypt(&ctx, dst, decode, strlen(pt));
	rc4_decrypt(&ctx, new_str, decode, strlen(new_str));
	printf("decode: len = %d, %s \n", i, decode);

	return 0;
}

#endif

