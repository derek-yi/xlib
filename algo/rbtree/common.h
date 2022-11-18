#ifndef _DP_COMMON_H_
#define _DP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "rbtree.h"
#include "rbtree_augmented.h"


#define BUILD_IN_DPDK
#define INCLUDE_UDP_PAYLOAD

#define VOS_OK                  0
#define VOS_ERR                 (-1)

#define XLOG_MSG      		    1
#define XLOG_DEBUG      		2
#define XLOG_INFO       		3
#define XLOG_WARN       		4
#define XLOG_ERROR      		5

extern int my_log_level;

int _xlog(const char *func, int line, int level, const char *format, ...);

#define xlog_msg(...)       _xlog(__func__, __LINE__, XLOG_MSG, __VA_ARGS__)
#define xlog_debug(...)     _xlog(__func__, __LINE__, XLOG_DEBUG, __VA_ARGS__)
#define xlog_info(...)      _xlog(__func__, __LINE__, XLOG_INFO, __VA_ARGS__)
#define xlog_warn(...)      _xlog(__func__, __LINE__, XLOG_WARN, __VA_ARGS__)
#define xlog_err(...)       _xlog(__func__, __LINE__, XLOG_ERROR, __VA_ARGS__)

uint32_t rte_rss_hash(uint32_t *input_tuple, uint32_t input_len);

uint32_t dict_str_hash(const char *key, int len);

int sys_node_readstr(const char *node_str, char *rd_buf, int buf_len);
int sys_node_read(const char *node_str, int *value);
int sys_node_read64(const char *node_str, long *value);
void fmt_ip_str(char *fmt_str, uint32_t ip_addr);
void fmt_time_str(char *time_str, int max_len);

void vos_msleep(uint32_t milliseconds);
int vos_run_cmd(const char *format, ...);
int vos_bind_cpu(int cpu_id);

#if 1

typedef int (*node_cmp)(void * a, void * b);

typedef int (*node_proc)(void * a, void *cookie);

#define BYTE_ALIGN(size, align) (((size)+(align)-1)&(~((align)-1)))

typedef struct gen_type {
    struct rb_node my_node;
    void *my_data;
}gen_type_t;

typedef struct gen_table {
    struct rb_root my_root;
	uint32_t count;
}gen_table_t;

struct gen_type *rbtree_search(gen_table_t *tree, void *key, node_cmp func);
int rbtree_insert(gen_table_t *tree, struct gen_type *data, node_cmp func);
int rbtree_delete(gen_table_t *tree, void *key, node_cmp func);
void rbtree_walk(gen_table_t *tree, node_proc func, void *cookie);
int rbtree_count(gen_table_t *tree);


#endif



#endif

