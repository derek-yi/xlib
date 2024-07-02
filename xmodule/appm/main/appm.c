#include "xmodule.h"
#include "appm.h"

int main()
{
	xmodule_init("appm", 0, NULL, NULL);
	
#if 0
	cli_main_task(NULL);
#else
	telnet_task_init();
	while(1) sleep(3);
#endif

	return 0;
}
