#include "xmodule.h"
#include "apps.h"

int main()
{
	xmodule_init("APPS", 1, NULL, NULL);
	
	cli_main_task(NULL);
	
	return 0;
}


