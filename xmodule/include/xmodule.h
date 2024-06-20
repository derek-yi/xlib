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

/****************************************************************************************
 * xmodule
 ****************************************************************************************/
#define DEF_CONFIG_FILE			"/home/config/top_cfg.txt"
#define TX_CONFIG_FILE			"/home/config/tx_atten_cfg.txt"
#define TX_COMP_FILE			"/home/config/tx_comp_cfg.txt"
#define WIFI_CONFIG_FILE		"/home/config/wpa_supplicant.conf"
#define DEF_LICENSE_FILE		"/home/config/raw_license.bin"
#define GNB_CFG_FILE			"/home/config/gnb_cfg.json"
#define GAIN_ADJUST_CONFIG		"/home/config/gain_adjust.txt"
#define EXT_CFG_FILE			"/home/config/ext_cfg.txt"


int xmodule_init(char *app_name, int mode, char *log_file, char *cfg_file);

char *get_app_name(void);

int get_app_role(void);

int app_in_master(void);

/****************************************************************************************
 * xx
 ****************************************************************************************/

#endif
