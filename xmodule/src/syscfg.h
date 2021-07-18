#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_



int sys_conf_set(char *key_str, char *value);

char* sys_conf_get(char *key_str);

int sys_conf_geti(char *key_str);

int parse_json_cfg(char *json_file);

int store_json_cfg(char *file_name);

int sys_conf_show(void);

#endif

