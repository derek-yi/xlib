
#include "xmodule.h"

#if 1

typedef struct 
{
    char    *cfg_file;
    char    *build_time;
    char    *app_name;
    int     app_role;
}SYS_CONF_PARAM;

SYS_CONF_PARAM xmodule_conf = {NULL};

char *get_app_name(void)
{   
    return xmodule_conf.app_name;
}

int cli_sys_cfg_proc(int argc, char **argv)
{
    int cp_len;

    if (argc < 2) {
        vos_print("usage: \r\n");
        vos_print("  cfg show               -- list cfg \r\n");
        vos_print("  cfg set <key> <value>  -- add cfg \r\n");
        vos_print("  cfg del <key>          -- delete cfg \r\n");
        vos_print("  cfg save               -- save cfg \r\n");
        vos_print("  cfg clear              -- clear saved file \r\n");
        return VOS_OK;
    }

    cp_len = strlen(argv[1]);
    if (!strncasecmp(argv[1], "show", cp_len)) {
        sys_conf_show();
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "set", cp_len)) {
        if (argc < 4) {
            vos_print("invalid param \r\n");
            return CMD_ERR_PARAM;
        }
        sys_conf_set(argv[2], argv[3]);
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "delete", cp_len)) {
        if (argc < 3) {
            vos_print("invalid param \r\n");
            return CMD_ERR_PARAM;
        }
        sys_conf_delete(argv[2]);
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "save", cp_len)) {
        store_json_cfg(xmodule_conf.cfg_file);
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "clear", cp_len)) {
        unlink(xmodule_conf.cfg_file);
        return VOS_OK;
    }
    
    return VOS_OK;
}

#endif

int cli_xlog_level(int argc, char **argv);

void xmodule_cmd_init(void)
{
    cli_cmd_reg("cfg",      "sys cfg operation",            &cli_sys_cfg_proc);
    cli_cmd_reg("xlog",     "xlog level config",            &cli_xlog_level);
}

//https://tenfy.cn/2017/09/16/only-one-instance/
#include <sys/file.h>
int single_instance_check(char *file_path)
{
    int fd = open(file_path, O_WRONLY|O_CREAT);
    if (fd < 0) {
        printf("open %s failed\n", file_path);
        return VOS_ERR;
    }

    int err = flock(fd, LOCK_EX|LOCK_NB);
    if (err == -1) {
        printf("lock failed\n");
        return VOS_ERR;
    }

    printf("lock success\n");
    //flock(fd, LOCK_UN);

    return VOS_OK;
}

int xmodule_init(char *app_name, char *log_file)
{
    char *cfg_name;
	char *json_file = "/home/config/top_cfg.json";

	xmodule_conf.cfg_file = strdup(json_file);
	if (access(json_file, F_OK) == 0) {
		if (parse_json_cfg(json_file) != VOS_OK) {
			printf("invalid json cfg \r\n");
			return VOS_ERR;
		}
	}
    
    cfg_name = sys_conf_get("app_name");
    if (cfg_name != NULL) {
        xmodule_conf.app_name = strdup(cfg_name);
    } else {
    	xmodule_conf.app_name = strdup(app_name);
    }
	
    xmodule_conf.app_role = sys_conf_geti("app_role");

    xlog_init(log_file);
	sys_conf_set("log_file", log_file);
	xlog_info("top cfg: %s", xmodule_conf.cfg_file);
	
    devm_msg_init(xmodule_conf.app_name, xmodule_conf.app_role);
	
	cli_cmd_init();
    xmodule_cmd_init();
    if (sys_conf_geti("telnet_enable")) {
        telnet_task_init();
    }
    if (sys_conf_geti("cli_enable")) {
        cli_task_init();
    }

    return VOS_OK;
}

#ifdef MAKE_APP

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: %s <app_name> \r\n", argv[0]);
		return VOS_OK;
	}

	sys_conf_seti("cli_enable", 1);
	xmodule_init(argv[1], NULL);

	while(1) sleep(3);
	return VOS_OK;
}

#endif

