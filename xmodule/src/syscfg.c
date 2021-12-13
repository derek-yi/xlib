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

static pthread_mutex_t this_mutex = PTHREAD_MUTEX_INITIALIZER;

int sys_conf_set(char *key_str, char *value)
{
    SYS_CFG_S *p;

    if ( (key_str == NULL) || (value == NULL) ) {
        return -1;
    }

	pthread_mutex_lock(&this_mutex);
    p = my_syscfg;
    while (p != NULL) {
        if ( !strcmp(key_str, p->key) ) { 
			// update 
			if (p->value) free(p->value);
            p->value = strdup(value);
			pthread_mutex_unlock(&this_mutex);
            return 0;
        }
        p = p->next;
    }

	//new
    p = (SYS_CFG_S *)malloc(sizeof(SYS_CFG_S));
    if (p == NULL) {
		pthread_mutex_unlock(&this_mutex);
        return -1;
    }

    p->key = strdup(key_str);
    p->value = strdup(value);
    p->next = my_syscfg;
    my_syscfg = p;

	pthread_mutex_unlock(&this_mutex);
    return 0;
}

int sys_conf_delete(char *key_str)
{
    SYS_CFG_S *p;
	SYS_CFG_S *prev;

    if (key_str == NULL) {
        return -1;
    }

	pthread_mutex_lock(&this_mutex);
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
			pthread_mutex_unlock(&this_mutex);
            return 0;
        }
		prev = p;
        p = p->next;
    }

	pthread_mutex_unlock(&this_mutex); 
    return 0;
}

char* sys_conf_get(char *key_str)
{
    SYS_CFG_S *p;

    if (key_str == NULL) return NULL;

    p = my_syscfg;
    while (p != NULL) {
        if ( !strcmp(key_str, p->key) ) {
            return p->value;
        }
        p = p->next;
    }

    return NULL;
}

int sys_conf_seti(char *key_str, int value)
{
	char value_str[64];

	sprintf(value_str, "0x%x", value);
	return sys_conf_set(key_str, value_str);
}

int sys_conf_geti(char *key_str)
{
    char *value_str;

	value_str = sys_conf_get(key_str);
    if (value_str == NULL) return 0;

    return strtol(value_str, NULL, 0);
}

int sys_conf_show(void)
{
    SYS_CFG_S *p;

    p = my_syscfg;
    while (p != NULL) {
        if ( p->key && p->value ) {
            vos_print("%-24s %s \r\n", p->key, p->value);
        }
        p = p->next;
    }    

    return VOS_OK;
}

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

    pthread_mutex_lock(&this_mutex);
    p = my_syscfg;
    while (p != NULL) {
        cJSON_AddItemToObject(root_tree, p->key, cJSON_CreateString(p->value));
        p = p->next;
    }
    pthread_mutex_unlock(&this_mutex);

    out = cJSON_Print(root_tree);
    if (out) {
        ret = json_write_file(file_name, out, strlen(out));
        vos_print("file content: \r\n %s \r\n", out);
    } 

    if (out != NULL) free(out);
    if (root_tree != NULL) cJSON_Delete(root_tree);
    
    return ret;
}

#ifndef MAKE_XLIB

int main()
{
	vos_print("---------------------------------\r\n");
	sys_conf_set("aa", "100");
	sys_conf_set("bb", "200");
	sys_conf_set("cc", "300");
	sys_conf_show();
	
	vos_print("---------------------------------\r\n");
	sys_conf_set("aa", "101");
	sys_conf_delete("bb");	
	sys_conf_show();

	vos_print("---------------------------------\r\n");
	store_json_cfg("./sysconf.json");
	sys_conf_delete("aa");	
	sys_conf_delete("cc");
	parse_json_cfg("./sysconf.json");

	vos_print("---------------------------------\r\n");
	sys_conf_show();

    return VOS_OK;
}

#endif


