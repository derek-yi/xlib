#include "xmodule.h"
#include "appm.h"

int main()
{
	xmodule_init("appm", 0, NULL, NULL);

	telnet_task_init(2300);
	while(1) sleep(3);

	return 0;
}
