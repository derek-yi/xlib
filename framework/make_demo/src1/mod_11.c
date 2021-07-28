
#include "pub1.h"
#include "mod_11.h"


int mod11_func(int param)
{
	return param + 10;
}

int mod11_init()
{
	int ret = 0;
	
	printf("mod11_init: %d \r\n", VERSION_NUM);
	
	return ret;
}