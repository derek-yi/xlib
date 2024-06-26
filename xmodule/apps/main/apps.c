#include "xmodule.h"
#include "apps.h"

int main()
{
	xmodule_init("apps", 1, NULL, NULL);
	sys_conf_set("xmsg_server", "127.0.0.1");

#if 1
	cli_main_task(NULL);
#else
	while(1) sleep(3);
#endif
	
	return 0;
}


