#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_

int sys_conf_set(char *key_str, char *value);

int sys_conf_seti(char *key_str, int value);

char* sys_conf_get(char *key_str);

int sys_conf_delete(char *key_str);

int sys_conf_geti(char *key_str, int def_val);

int parse_json_cfg(char *json_file);

int store_json_cfg(char *file_name);

int sys_conf_show(void);

int cfgfile_load_file(char *file_name);

int cfgfile_store_file(char *old_file, char *new_file);

#endif

