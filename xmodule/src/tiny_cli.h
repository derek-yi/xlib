#ifndef _TINY_CLI_H_
#define _TINY_CLI_H_

#define INCLUDE_CONSOLE
#define INCLUDE_TELNETD
#define CHECK_AMBIGUOUS
//#define CLI_PWD_CHECK 

#define CMD_OK                  0x00
#define CMD_ERR                 0x01
#define CMD_ERR_PARAM           0x02
#define CMD_ERR_NOT_MATCH       0x03
#define CMD_ERR_AMBIGUOUS       0x04
#define CMD_ERR_EXIT            0x99

#define TELNETD_LISTEN_PORT     2300

int cli_telnet_active(void);

void cli_cmd_init(void);

int cli_task_init(void);

int telnet_task_init(void);

typedef int (*CLI_OUT_CB)(void *cookie, char *buff);

int cli_set_output_cb(CLI_OUT_CB cb, void *cookie);

int cli_cmd_exec(char *buff);

typedef int (* CMD_FUNC)(int argc, char **argv);

int cli_cmd_reg(const char *cmd, const char *help, CMD_FUNC func);

int vos_print(const char * format,...);



#endif

