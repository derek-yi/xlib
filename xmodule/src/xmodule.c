
#include "xmodule.h"

#if 1

typedef struct 
{
    char    *top_cfg;
    char    *build_time;
    char    *app_name;
    int     app_role;
}SYS_CONF_PARAM;


SYS_CONF_PARAM xmodule_conf;

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

    if (!strncasecmp(argv[1], "save", cp_len)) {
        store_json_cfg("./top_cfg.json");
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "clear", cp_len)) {
        //unlink("./top_cfg.json");
        return VOS_OK;
    }
    
    return VOS_OK;
}

#endif

void xmodule_cmd_init(void)
{
    cli_cmd_init();

    cli_cmd_reg("cfg",      "sys cfg operation",            &cli_sys_cfg_proc);
}

int xmodule_init(char *json_file)
{
    char *cfg_file = "./top_cfg.json";
    char *app_name;

    if (access(cfg_file, F_OK) != 0) {
        cfg_file = json_file;  //as init cfg
    }
    
    if (parse_json_cfg(cfg_file) != VOS_OK) {
        printf("invalid json cfg \r\n");
        return VOS_ERR;
    }
    
    app_name = sys_conf_get("app_name");
    if (app_name == NULL) {
        printf("no app_name in cfg file \r\n");
        return VOS_ERR;
    }
    xmodule_conf.app_name = strdup(app_name);
    xmodule_conf.app_role = sys_conf_geti("app_role");

    xlog_init(app_name);
    
    devm_msg_init(xmodule_conf.app_name, xmodule_conf.app_role);

    xmodule_cmd_init();
    if (sys_conf_geti("telnet_enable")) {
        telnet_task_init();
    }
    if (sys_conf_geti("cli_enable")) {
        cli_task_init();
    }

    return VOS_OK;
}

