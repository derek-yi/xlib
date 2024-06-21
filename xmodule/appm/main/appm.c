#include "xmodule.h"
#include "appm.h"

int main()
{
	xmodule_init("MAIN", 0, NULL, NULL);
	
	cli_main_task(NULL);

	return 0;
}
