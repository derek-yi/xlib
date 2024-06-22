#ifndef _VOS_H_
#define _VOS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#define VOS_OK      		0
#define VOS_ERR     		(1)
#define VOS_E_PARAM     	(2)
#define VOS_E_NONEXIST     	(3)
#define VOS_E_FILE     		(4)
#define VOS_E_SOCK     		(5)
#define VOS_E_MALLOC   		(6)

/************************************************************************************/
#define LOCAL_IP_ADDR               "127.0.0.1"

#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif

#ifndef T_DESC
#define T_DESC(x, y)    y
#endif

#ifdef __DEBUG
#define x_perror(x)   perror(x)
#else
#define x_perror(x)	 
#endif

#define max(x, y) \
	(((x) > (y)) ? (x) : (y))
	
#define max_t(type, x, y) \
	(type)max((type)(x), (type)(y))

#define clamp(val, min_val, max_val) \
	(max(min((val), (max_val)), (min_val)))
	
#define clamp_t(type, val, min_val, max_val) \
	(type)clamp((type)(val), (type)(min_val), (type)(max_val))

#define swap_t(x, y) \
	{typeof(x) _tmp_ = (x); (x) = (y); (y) = _tmp_;}

#define round_up(x,y) \
		(((x)+(y)-1)/(y))

typedef struct
{
    char name[32];  
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
}CPU_OCCUPY;

typedef struct
{
    long total;
    long free;
    long buffers;
    long cached;
    long swap_cached;
    long swap_total;
    long swap_free;
    long available;
}MEM_OCCUPY;

unsigned int hweight32(unsigned int w);
short mk_num16(char high, char low);
int mk_num32(char b0, char b1, char b2, char b3);
int mk_boundary(int frame, int slot, int symbol);
int list_max_index2(uint64_t *list, int size);
int xlog_save_list(char *file_name, int *list, int cnt);
int xlog_save_list2(char *file_name, uint64_t *list, int cnt);

uint32 devmem_read(uint64_t mem_addr);
uint32 devmem_write(uint64_t mem_addr, uint32 writeval);

int cfgfile_read_str(char *file_name, char *key_str, char *val_buf, int buf_len);

int cfgfile_write_str(char *file_name, char *key_str, char *val_str);

int sys_read_pipe(char *cmd_str, char *buff, int buf_len);

int sys_node_readstr(char *node_str, char *rd_buf, int buf_len);

int sys_node_read(char *node_str, int *value);

int sys_node_writestr(char *node_str, char *wr_buf);

int sys_node_write(char *node_str, int value);

typedef int (* timer_cb)(void *param);

typedef struct 
{
    uint32      enable;
    uint32      interval;
    uint32      run_cnt;
    timer_cb    cb_func;
    void       *cookie;
}TIMER_INFO_S;

int vos_create_timer(timer_t *ret_tid, int interval, int repeat, timer_cb callback, void *param);

int vos_delete_timer(timer_t timerid);

int vos_file_exist(char *file_path);

long vos_file_size(char *file_path);

int vos_bind_to_cpu(int cpu_id);

int vos_set_self_name(char *thread_name);

void fmt_time_str(char *time_str, int max_len);

void vos_msleep(uint32 milliseconds);

int vos_run_cmd(const char *format, ...);

int vos_print(const char * format,...);

void vos_msleep(uint32 milliseconds);

int vos_sem_wait(void *sem, uint32 msecs);

int vos_sem_clear(void *sem_id);

int tty_set_raw(int fd, int baud);

int fpga_read(uint32_t address);
int fpga_write(uint32_t address, uint32_t value);

int axi_spi_read(uint32_t address);
int axi_spi_write(uint32_t address, uint32_t data);

int get_local_ip(char *if_name);
int get_local_mac(char *if_name, char *mac_addr);

int32_t gpio_direction_output(int gpio_num, uint8_t value);
int32_t gpio_set_value(int gpio_num, uint8_t value);

unsigned short CRC16_CCITT(unsigned char *puchMsg, unsigned int usDataLen);
unsigned short vos_crc16(unsigned char *d, unsigned short len, unsigned short initcrc);

int utc_tm_convert(struct tm *utc_tm, struct tm *bj_tm);

long vos_get_ts(void);

void get_cpu_occupy(CPU_OCCUPY *cpu_occupy);
int vos_get_cpu_occupy(CPU_OCCUPY *o, CPU_OCCUPY *n);
int vos_get_mem_occupy(void);

int vos_send_udp_pkt(char *dst_ipaddr, int dst_port, void *tx_buff, int len);

#endif


