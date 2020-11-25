
#ifndef _DEV_CMD_H_
#define _DEV_CMD_H_


//#define CLI_PWD_CHECK 

#define TELNETD_LISTEN_PORT     2300
#define INCLUDE_CONSOLE
#define INCLUDE_TELNETD


#define CMD_OK                  0x00
#define CMD_ERR                 0x01
#define CMD_ERR_PARAM           0x02
#define CMD_ERR_NOT_MATCH       0x03
#define CMD_ERR_AMBIGUOUS       0x04
#define CMD_ERR_EXIT            0x99

typedef int (* CMD_FUNC)(int argc, char **argv);

int cli_cmd_reg(const char *cmd, const char *help, CMD_FUNC func);

void cli_cmd_init(void);

void cli_main_task(void);

int telnet_task_init(void);

int vos_print(const char * format,...);

#endif

