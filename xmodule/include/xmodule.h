#ifndef _XMODULE_H_
#define _XMODULE_H_

#include "vos.h"
#include "xlog.h"
#include "tiny_cli.h"
#include "cJSON.h"
#include "syscfg.h"
#include "xmsg.h"

/****************************************************************************************
 * config
 ****************************************************************************************/
#define INCLUDE_JSON_CFGFILE

#define DEF_CONFIG_FILE			"/home/config/top_cfg.txt"

/****************************************************************************************
 * xmodule
 ****************************************************************************************/
#define APP_ROLE_MASTER             0
#define APP_ROLE_SLAVE              1

int xmodule_init(char *app_name, int mode, char *log_file, char *cfg_file);

char *get_app_name(void);

int get_app_role(void);

int app_in_master(void);

/****************************************************************************************
 * xx
 ****************************************************************************************/

#endif
