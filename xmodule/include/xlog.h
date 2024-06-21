#ifndef _XLOG_H_
#define _XLOG_H_

#define XLOG_BUFF_MAX           2048
#define OAM_XLOG_FWD_PORT       11030

#define XLOG_DEBUG      		1
#define XLOG_TRACE              2
#define XLOG_INFO       		3
#define XLOG_WARN       		4
#define XLOG_ERROR      		5
#define XLOG_EXTERN      		0x800

int xlog_init(char *app_name);

int _xlog(const char *func, int line, int level, const char *format, ...);

int xlog_cmd_set_level(int argc, char **argv);

#define xlog(level, ...)  	_xlog(__func__, __LINE__, level, __VA_ARGS__)

#define xlog_debug(...)     xlog(XLOG_DEBUG, __VA_ARGS__)
#define xlog_trace(...)     xlog(XLOG_TRACE, __VA_ARGS__)
#define xlog_info(...)      xlog(XLOG_INFO, __VA_ARGS__)
#define xlog_warn(...)      xlog(XLOG_WARN, __VA_ARGS__)
#define xlog_err(...)       xlog(XLOG_ERROR, __VA_ARGS__)

#endif
