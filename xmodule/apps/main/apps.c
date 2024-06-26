#include "xmodule.h"
#include "apps.h"

int main()
{
	xmodule_init("apps", 1, NULL, NULL);
	sys_conf_set("xmsg_server", "127.0.0.1");
	
	cli_main_task(NULL);
	
	return 0;
}


