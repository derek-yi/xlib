
#include "xmodule.h"

char *get_app_name(void)
{   
    return sys_conf_get("app_name");
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
		#ifdef INCLUDE_JSON_CFGFILE
        store_json_cfg(cfg_file);
        #else
		cfgfile_store_file(cfg_file, cfg_file);
		#endif
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "clear", cp_len)) {
        unlink(cfg_file);
        return VOS_OK;
    }
    
    return VOS_OK;
}

int cli_xlog_level(int argc, char **argv);

void xmodule_cmd_init(void)
{
    cli_cmd_reg("cfg",      "sys cfg operation",            &cli_sys_cfg_proc);
    cli_cmd_reg("xlog",     "xlog level config",            &cli_xlog_level);
}

int xmodule_init(char *app_name, char *log_file)
{
	int app_role;

	if (access(DEF_CONFIG_FILE, F_OK) == 0) {
		#ifdef INCLUDE_JSON_CFGFILE
		if (parse_json_cfg(DEF_CONFIG_FILE) != VOS_OK) {
			printf("invalid json cfg \r\n");
			return VOS_ERR;
		}
		#else
		if (cfgfile_load_file(DEF_CONFIG_FILE) != VOS_OK) {
			printf("invalid cfg file \r\n");
			return VOS_ERR;
		}
		#endif
	}

	sys_conf_set("sys_cfgfile", DEF_CONFIG_FILE);
    if (sys_conf_get("app_name") == NULL) {
        sys_conf_set("app_name", "app_main");
    }
	
    xlog_init(log_file);
	sys_conf_set("log_file", log_file);
	xlog_info("------------------------------------------------------------");
	xlog_info("top cfg: %s", DEF_CONFIG_FILE);
	
    app_role = sys_conf_geti("app_role", 1);
    devm_msg_init(sys_conf_get("app_name"), app_role);
	
	cli_cmd_init();
    xmodule_cmd_init();
    if (sys_conf_geti("telnet_enable", 1)) {
        telnet_task_init();
    }
    if (sys_conf_geti("cli_enable", 0)) {
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

