
#ifndef _DEV_CMD_H_
#define _DEV_CMD_H_


#define VOS_OK          0
#define VOS_ERR         1

#ifndef uint32
#define uint32 unsigned int
#endif

typedef int (* FUNC_ENTRY)(int argc, char **argv);

void cli_cmd_init(void);
void cli_main_task(void);

int cli_cmd_reg(char *cmd, char *help, FUNC_ENTRY func);

#endif

