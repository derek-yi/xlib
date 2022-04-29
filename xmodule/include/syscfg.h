#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_

//#define INCLUDE_JSON_CFGFILE

int sys_conf_set(char *key_str, char *value);

char* sys_conf_get(char *key_str);

int sys_conf_delete(char *key_str);

int sys_conf_geti(char *key_str, int def_val);

int parse_json_cfg(char *json_file);

int store_json_cfg(char *file_name);

int sys_conf_show(void);

int cfgfile_load_file(char *file_name);

int cfgfile_store_file(char *cfg_file, char *bak_file);

int cfgfile_reload_file(char *def_file, char *cur_file);

#endif

