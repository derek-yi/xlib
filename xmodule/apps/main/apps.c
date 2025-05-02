#include "xmodule.h"
#include "apps.h"

int main()
{
	sys_conf_set("xmsg_server", "127.0.0.1");
	xmodule_init("apps", 1, NULL, NULL);

#if 0
	telnet_task_init(2310);
	while(1) sleep(3);
#else
	cli_main_task(NULL);
#endif

	return 0;
}


