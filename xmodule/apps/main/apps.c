#include "xmodule.h"
#include "apps.h"

int main()
{
	xmodule_init("apps", 1, NULL, NULL);
	sys_conf_set("xmsg_server", "127.0.0.1");

	telnet_task_init(2310);
	while(1) sleep(3);
	
	return 0;
}


