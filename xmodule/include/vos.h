#ifndef _VOS_H_
#define _VOS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

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
#define VOS_ERR     		(-1)
#define VOS_E_PARAM     	(-2)
#define VOS_E_NONEXIST     	(-3)
#define VOS_E_FILE     		(-4)
#define VOS_E_SOCK     		(-5)
#define VOS_E_MALLOC   		(-6)

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

#ifdef ROCKCHIP_BOARD   //FPGA_BAR_ADDR
#define FPGA_BASE_ADDR           0x900000000
#else
#define FPGA_BASE_ADDR           0x280000000
#endif

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

void fmt_time_str(char *time_str, int max_len);

void vos_msleep(uint32 milliseconds);

int vos_run_cmd(const char *format, ...);

int vos_print(const char * format,...);

void vos_msleep(uint32 milliseconds);

int vos_sem_wait(void *sem, uint32 msecs);

#endif


