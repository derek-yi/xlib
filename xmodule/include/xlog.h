#ifndef _XLOG_H_
#define _XLOG_H_

#define XLOG_BUFF_MAX           1024

#define XLOG_DEBUG      		1
#define XLOG_INFO       		2
#define XLOG_WARN       		3
#define XLOG_ERROR      		4

int xlog_print_file(char *filename);

int xlog_init(char *app_name);

int _xlog(const char *func, int line, int level, const char *format, ...);

#define xlog(level, ...)  	_xlog(__func__, __LINE__, level, __VA_ARGS__)

#define xlog_debug(...)     xlog(XLOG_DEBUG, __VA_ARGS__)
#define xlog_info(...)      xlog(XLOG_INFO, __VA_ARGS__)
#define xlog_warn(...)      xlog(XLOG_WARN, __VA_ARGS__)
#define xlog_err(...)       xlog(XLOG_ERROR, __VA_ARGS__)

#endif
