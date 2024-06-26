#include "xmodule.h"

int app_role = APP_ROLE_SLAVE;

char *get_app_name(void)
{   
    return sys_conf_get("app_name");
}

int get_app_role(void)
{
    return app_role;
}

int app_in_master(void)
{
    return (app_role == APP_ROLE_MASTER);
}

int cli_sys_cfg_proc(int argc, char **argv)
{
    int cp_len;
	char *cfg_file = sys_conf_get("sys_cfgfile");

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
		vos_print("%-24s %s %s\r\n", "build_time", __DATE__, __TIME__);
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
		cfgfile_store_file(cfg_file, cfg_file);
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "clear", cp_len)) {
        unlink(cfg_file);
        return VOS_OK;
    }
    
    return VOS_OK;
}

void xmodule_cmd_init(void)
{
    cli_cmd_reg("cfg",      "sys cfg operation",            &cli_sys_cfg_proc);
    cli_cmd_reg("xlog",     "xlog level config",            &xlog_cmd_set_level);
}

int xmodule_init(char *app_name, int mode, char *log_file, char *cfg_file)
{
	if (access(cfg_file, F_OK) == 0) {
		if (cfgfile_load_file(cfg_file) != VOS_OK) {
			printf("invalid cfg file \r\n");
			return VOS_ERR;
		}
        sys_conf_set("sys_cfgfile", DEF_CONFIG_FILE);
	}

	app_role = mode;
    if (app_name == NULL) {
        sys_conf_set("app_name", "app_main");    } else {
		sys_conf_set("app_name", app_name);	}

    xlog_init(log_file);
	sys_conf_set("log_file", log_file);
	xlog_info("------------------------------------------------------------");
    
	cli_cmd_init();
    xmodule_cmd_init();
	devm_msg_init(app_role == APP_ROLE_MASTER);

    if (sys_conf_geti("telnet_enable", 1)) {
        telnet_task_init();
    }
    if (sys_conf_geti("cli_enable", 0)) {
        cli_task_init();
    }

    return VOS_OK;
}



