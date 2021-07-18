
#ifndef _XMODULE_H_
#define _XMODULE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

#include "vos.h"
#include "xlog.h"
#include "tiny_cli.h"
#include "cJSON.h"
#include "syscfg.h"
#include "xmsg.h"

/****************************************************************************************
 * xmodule
 ****************************************************************************************/
#define APP_ROLE_SLAVE          0
#define APP_ROLE_MASTER         1

int xmodule_init(char *json_file);

char* sys_conf_get(char *key_str);

int sys_conf_geti(char *key_str);

int sys_conf_set(char *key_str, char *value);

char *get_app_name(void);

/****************************************************************************************
 * xx
 ****************************************************************************************/

#endif
