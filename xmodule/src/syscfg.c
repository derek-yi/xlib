#include "vos.h"
#include <pthread.h>

#include "cJSON.h"
#include "syscfg.h"


#ifndef MAKE_XLIB

#define vos_print   printf

#endif

typedef struct _SYS_CFG{
    struct _SYS_CFG *next;
    char    *key;
    char    *value;
}SYS_CFG_S;

static SYS_CFG_S *my_syscfg = NULL;

static pthread_mutex_t syscfg_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cfgfile_mutex = PTHREAD_MUTEX_INITIALIZER;

#if 1
int sys_conf_set(char *key_str, char *value)
{
    SYS_CFG_S *p;

    if ( (key_str == NULL) || (value == NULL) ) {
        return VOS_E_PARAM;
    }

	pthread_mutex_lock(&syscfg_mutex);
    p = my_syscfg;
    while (p != NULL) {
        if ( !strcmp(key_str, p->key) ) {  //update 
			if (p->value) free(p->value);
            p->value = strdup(value);
			pthread_mutex_unlock(&syscfg_mutex);
            return VOS_OK;
        }
        p = p->next;
    }

	//new
    p = (SYS_CFG_S *)malloc(sizeof(SYS_CFG_S));
    if (p == NULL) {
		pthread_mutex_unlock(&syscfg_mutex);
        return VOS_E_MALLOC;
    }

    p->key = strdup(key_str);
    p->value = strdup(value);
    p->next = my_syscfg;
    my_syscfg = p;

	pthread_mutex_unlock(&syscfg_mutex);
    return VOS_OK;
}

int sys_conf_delete(char *key_str)
{
    SYS_CFG_S *p;
	SYS_CFG_S *prev;

    if (key_str == NULL) {
        return VOS_E_PARAM;
    }

	pthread_mutex_lock(&syscfg_mutex);
    p = my_syscfg;
	prev = NULL;
    while (p != NULL) {
        if ( !strcmp(key_str, p->key) ) {
            if (prev) {
				prev->next = p->next;	
            } else {
				my_syscfg = p->next;
            }

			if (p->key) free(p->key);
			if (p->value) free(p->value);
			free(p);
			pthread_mutex_unlock(&syscfg_mutex);
            return VOS_OK;
        }
		prev = p;
        p = p->next;
    }

	pthread_mutex_unlock(&syscfg_mutex); 
    return VOS_E_NONEXIST;
}

char* sys_conf_get(char *key_str)
{
    SYS_CFG_S *p;

    if (key_str == NULL) return NULL;

	pthread_mutex_lock(&syscfg_mutex);
    p = my_syscfg;
    while (p != NULL) {
        if ( !strcmp(key_str, p->key) ) {
			pthread_mutex_unlock(&syscfg_mutex);
            return p->value;
        }
        p = p->next;
    }

	pthread_mutex_unlock(&syscfg_mutex);
    return NULL;
}

int sys_conf_seti(char *key_str, int value)
{
	char value_str[64];

	sprintf(value_str, "0x%x", value);
	return sys_conf_set(key_str, value_str);
}

int sys_conf_geti(char *key_str, int def_val)
{
    char *value_str;

	value_str = sys_conf_get(key_str);
    if (value_str == NULL) return def_val;

    return strtol(value_str, NULL, 0);
}

int sys_conf_show(void)
{
    SYS_CFG_S *p;

	pthread_mutex_lock(&syscfg_mutex);
    p = my_syscfg;
    while (p != NULL) {
        if ( p->key && p->value ) {
            vos_print("%-24s %s \r\n", p->key, p->value);
        }
        p = p->next;
    }    

	pthread_mutex_unlock(&syscfg_mutex);
    return VOS_OK;
}
#endif

#ifdef INCLUDE_JSON_CFGFILE
int parse_json_cfg(char *json_file)
{
    char *json = NULL;
    cJSON* root_tree;
    int list_cnt;

    printf("load conf %s ...\r\n", json_file);
    json = json_read_file(json_file);
	if (json == NULL) {
		printf("file content is null\r\n");
		return VOS_ERR;
	}

	root_tree = cJSON_Parse(json);
	if (root_tree == NULL) {
		printf("parse json file fail\r\n");
        return VOS_ERR;
	}

	list_cnt = cJSON_GetArraySize(root_tree);
	for (int i = 0; i < list_cnt; ++i) {
		cJSON* tmp_node = cJSON_GetArrayItem(root_tree, i);
        char num_str[64];

        if (tmp_node->valuestring) {
			sys_conf_set(tmp_node->string, tmp_node->valuestring);
        } else {
            sprintf(num_str, "%d", tmp_node->valueint);
            sys_conf_set(tmp_node->string, num_str);
        }
	}

    if (root_tree != NULL) {
        cJSON_Delete(root_tree);
    }
    
    return VOS_OK;
}

int store_json_cfg(char *file_name)
{
    cJSON* root_tree;
    int ret = VOS_ERR;
    char * out;
    SYS_CFG_S *p;

	if (file_name == NULL) return VOS_ERR;
	
    root_tree = cJSON_CreateObject();
    if (root_tree == NULL) return VOS_ERR;

    pthread_mutex_lock(&syscfg_mutex);
    p = my_syscfg;
    while (p != NULL) {
        cJSON_AddItemToObject(root_tree, p->key, cJSON_CreateString(p->value));
        p = p->next;
    }
    pthread_mutex_unlock(&syscfg_mutex);

    out = cJSON_Print(root_tree);
    if (out) {
        ret = json_write_file(file_name, out, strlen(out));
        vos_print("file content: \r\n %s \r\n", out);
    } 

    if (out != NULL) free(out);
    if (root_tree != NULL) cJSON_Delete(root_tree);
    
    return ret;
}
#endif

#if 1

#define LINE_BUF_SZ				256
#define MAX_CFG_LINE			1024
#define INVALID_CFG_CHAR(ch)	((ch) == ' ' || (ch) == '\t' || (ch) == '=' || (ch) == '\r' || (ch) == '\n' || (ch) == 0)

static char file_buff[MAX_CFG_LINE][LINE_BUF_SZ];

int cfgfile_read_str(char *file_name, char *key_str, char *val_buf, int buf_len)
{
	FILE *fp;
	int i, ret;
	char *ptr;
	char line_str[LINE_BUF_SZ];

    if (key_str == NULL || val_buf == NULL) return VOS_E_PARAM;

	pthread_mutex_lock(&cfgfile_mutex);
	fp = fopen(file_name, "r");
    if (fp == NULL) {
		x_perror("fopen");
		pthread_mutex_unlock(&cfgfile_mutex);
        return VOS_E_FILE;
    }

	ret = VOS_E_NONEXIST;
    while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
		ptr = &line_str[0];
		while ( *ptr == ' ' || *ptr == '\t' ) ptr++; //skip space/tab at head
		if (*ptr == '#') continue; //skip comment line
		
		if (memcmp(ptr, key_str, strlen(key_str)) == 0) {
			ptr = ptr +  strlen(key_str);
			while (*ptr == ' ' || *ptr == '\t' || *ptr == '=') ptr++; 
			for (i = 0; ; i++) {
				if ( INVALID_CFG_CHAR(ptr[i]) )
					break;
			}
			
			if (i > 0 && i < buf_len) {
				memset(val_buf, 0, buf_len);
				memcpy(val_buf, ptr, i);
				ret = VOS_OK;
			} 
			break;
		}
    }

	fclose(fp);
	pthread_mutex_unlock(&cfgfile_mutex);
    return ret;
}

int cfgfile_write_str(char *file_name, char *key_str, char *val_str)
{
	FILE *fp;
	char line_str[LINE_BUF_SZ];
	char *ptr;
	char *file_buff;
	int find_node = 0;
	int buf_ptr = 0;

	if (file_name == NULL || key_str == NULL) return -1;

	file_buff = (char *)malloc(MAX_CFG_LINE*LINE_BUF_SZ);
	if (file_buff == NULL) {
		x_perror("malloc");
		return VOS_E_MALLOC;
	}
	memset(file_buff, 0, MAX_CFG_LINE*LINE_BUF_SZ);

	pthread_mutex_lock(&cfgfile_mutex);
	fp = fopen(file_name, "r");
	if (fp != NULL) {
		while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
			ptr = &line_str[0];
			while ( *ptr == ' ' || *ptr == '\t' ) ptr++;
			
			if (memcmp(ptr, key_str, strlen(key_str)) == 0) {
				find_node = 1;
				if (val_str == NULL) {
					//printf("delete %s\n", key_str);
					snprintf(line_str, LINE_BUF_SZ, "%s = \n", key_str);
				} else {
					//printf("update %s to %s \n", key_str, val_str);
					snprintf(line_str, LINE_BUF_SZ, "%s = %s\n", key_str, val_str);
				}
			} 

			memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
			buf_ptr += strlen(line_str);
		}

		fclose(fp);
	}

	if ( (find_node == 0) && (val_str != NULL) ) {
		//printf("add %s = %s \n", key_str, val_str);
		snprintf(line_str, LINE_BUF_SZ, "%s = %s\n", key_str, val_str);
		memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
		buf_ptr += strlen(line_str);
	}

	fp = fopen(file_name, "w+");
	if (fp == NULL) {
		x_perror("fopen");
		free(file_buff);
		pthread_mutex_unlock(&cfgfile_mutex);
		return VOS_E_FILE;
	}
	
	fwrite(file_buff, buf_ptr, 1, fp);
	free(file_buff);
	fclose(fp);
	pthread_mutex_unlock(&cfgfile_mutex);
	return VOS_OK;
}

int cfgfile_load_file(char *file_name)
{
	FILE *fp;
	int i;
	char line_str[LINE_BUF_SZ];
	char key_str[LINE_BUF_SZ/2];
	char val_str[LINE_BUF_SZ/2];

	pthread_mutex_lock(&cfgfile_mutex);
	fp = fopen(file_name, "r");
	if (fp == NULL) {
		x_perror("fopen");
		pthread_mutex_unlock(&cfgfile_mutex);
		return VOS_E_FILE;
	}
	
	while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
		char *ptr = &line_str[0];
		while ( *ptr == ' ' || *ptr == '\t' ) ptr++; //skip space/tab at head
		if (*ptr == '#') continue; //skip comment line

		//get key_str
		for (i = 0; ;i++) {
			if ( INVALID_CFG_CHAR(ptr[i]) ) 
				break;
		}
		if (i == 0) continue;
		memset(key_str, 0, sizeof(key_str));
		memcpy(key_str, ptr, i);

		//get val_str
		ptr += i;
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '=') ptr++; 
		for (i = 0; ;i++) {
			if ( INVALID_CFG_CHAR(ptr[i]) ) 
				break;
		}
		if (i == 0) continue;
		memset(val_str, 0, sizeof(val_str));
		memcpy(val_str, ptr, i);

		//printf("load %s %s \n", key_str, val_str);
		sys_conf_set(key_str, val_str);
	}

	fclose(fp);
	pthread_mutex_unlock(&cfgfile_mutex);
	return VOS_OK;
}

int cfgfile_store_file(char *cfg_file, char *bak_file)
{
	FILE *fp;
    SYS_CFG_S *p;
	int find_key = 0;
	char key_str[LINE_BUF_SZ];
	int i, line_cnt = 0;
	int add_line = 0;

	if (cfg_file == NULL) return VOS_E_PARAM;
	if (bak_file != NULL) {
		snprintf(key_str, sizeof(key_str), "cp -rf %s %s", cfg_file, bak_file);
		vos_run_cmd(key_str);
	}

	pthread_mutex_lock(&cfgfile_mutex);
	fmt_time_str(key_str, LINE_BUF_SZ);
	sys_conf_set("last_update", key_str);

	fp = fopen(cfg_file, "r");
	if (fp != NULL) {
		memset(file_buff[line_cnt], 0, LINE_BUF_SZ);
		while (fgets(file_buff[line_cnt], LINE_BUF_SZ, fp) != NULL) { 
			char *ptr = file_buff[line_cnt];
			while ( *ptr == ' ' || *ptr == '\t' ) ptr++; //skip space/tab at head
			if (*ptr == '#') goto NEXT_LINE;

			//if key_str not exist in my_syscfg, delete it
			for (i = 0; ;i++) {
				if ( INVALID_CFG_CHAR(ptr[i]) ) 
					break;
			}
			if (i > 0) {
				memset(key_str, 0, sizeof(key_str));
				memcpy(key_str, ptr, i);
				if (sys_conf_get(key_str) == NULL) {
					//printf("delete %s \n", key_str);
					snprintf(file_buff[line_cnt], LINE_BUF_SZ, "%s = \n", key_str);
				}
			}

		NEXT_LINE:	
			line_cnt++;
			if (line_cnt >= MAX_CFG_LINE) break;
		}
		fclose(fp);
	}

	//printf("line_cnt %d \n", line_cnt);
    p = my_syscfg;
    while (p != NULL) {
        if ( p->key == NULL || p->value == NULL) {
            p = p->next;
			continue;
        }

		for ( i = 0, find_key = 0; i < line_cnt; i++) {
			char *ptr = file_buff[i];
			while ( *ptr == ' ' || *ptr == '\t' ) ptr++; //skip space/tab at head
			if ( (*ptr == '#') || (*ptr == 0) ) continue; //skip comment or blank line

			if (memcmp(ptr, p->key, strlen(p->key)) == 0) {
				//printf("update %s %s \n", p->key, p->value);
				snprintf(file_buff[i], LINE_BUF_SZ, "%s = %s\n", p->key, p->value);
				find_key = 1;
				break;
			}
		}

		if ( (find_key == 0) && (line_cnt + add_line < MAX_CFG_LINE) ) {
			//printf("add %s %s \n", p->key, p->value);
			snprintf(file_buff[line_cnt + add_line], LINE_BUF_SZ, "%s = %s\n", p->key, p->value);
			add_line++;
		}

		p = p->next;
		continue;
    }    

	fp = fopen(cfg_file, "w");
	if (fp == NULL) {
		x_perror("fopen");
		pthread_mutex_unlock(&cfgfile_mutex);
		return VOS_E_FILE;
	}
	
	for ( i = 0; i < add_line + line_cnt; i++) {
		fwrite(file_buff[i], strlen(file_buff[i]), 1, fp);
	}
	fclose(fp);

	pthread_mutex_unlock(&cfgfile_mutex);
	return VOS_OK;
}

int cfgfile_unit_test(void)
{
	int ret;
	char rd_buff[64];

	ret = cfgfile_write_str("./my_cfg.txt", "aa", "100");
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);
	ret = cfgfile_write_str("./my_cfg.txt", "bb", "200");
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);
	ret = cfgfile_write_str("./my_cfg.txt", "cc", "300");
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);
	ret = cfgfile_write_str("./my_cfg.txt", "cc", NULL);
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);
	
	ret = cfgfile_read_str("./my_cfg.txt", "aa", rd_buff, sizeof(rd_buff));
	if ( (ret != 0) || (strcmp(rd_buff, "100") != 0) ) printf("%d error %d \n", __LINE__, ret);

	cfgfile_write_str("./my_cfg.txt", "aa", "101");
	ret = cfgfile_read_str("./my_cfg.txt", "aa", rd_buff, sizeof(rd_buff));
	if ( (ret != 0) || (strcmp(rd_buff, "101") != 0) ) printf("%d error %d \n", __LINE__, ret);
	
	ret = cfgfile_read_str("./my_cfg.txt", "bb", rd_buff, sizeof(rd_buff));
	if ( (ret != 0) || (strcmp(rd_buff, "200") != 0) ) printf("%d error %d \n", __LINE__, ret);
	
	ret = cfgfile_read_str("./my_cfg.txt", "cc", rd_buff, sizeof(rd_buff));
	if ( ret == 0 ) printf("%d error %d \n", __LINE__, ret);

	ret = cfgfile_load_file("./my_cfg.txt");	
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);
	sys_conf_show();
	printf("------------------------------------\n");

	ret = strcmp(sys_conf_get("aa"), "101");
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);

	ret = strcmp(sys_conf_get("bb"), "200");
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);

	sys_conf_delete("bb");
	sys_conf_set("cc", "222");
	sys_conf_show();
	ret = cfgfile_store_file("./new_cfg.txt", "./bak_cfg.txt");	
	if ( ret != 0 ) printf("%d error %d \n", __LINE__, ret);

    return VOS_OK;
}

#endif

#ifndef MAKE_XLIB
//gcc syscfg.c vos.c -I../include -lpthread -lrt
int main()
{
#if XX
	vos_print("---------------------------------\r\n");
	sys_conf_set("aa", "100");
	sys_conf_set("bb", "200");
	sys_conf_set("cc", "300");
	sys_conf_show();
	
	vos_print("---------------------------------\r\n");
	sys_conf_set("aa", "101");
	sys_conf_delete("bb");	
	sys_conf_show();
#endif

#ifdef INCLUDE_JSON_CFGFILE
	vos_print("---------------------------------\r\n");
	store_json_cfg("./sysconf.json");
	sys_conf_delete("aa");	
	sys_conf_delete("cc");
	parse_json_cfg("./sysconf.json");
	vos_print("---------------------------------\r\n");
	sys_conf_show();
#endif

	cfgfile_unit_test();
    return VOS_OK;
}

#endif

