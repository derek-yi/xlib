
#include "pub1.h"

int main()
{
	int num = 1000;
	
	printf("app_main start \n");
	
	num = libx1_func(num);
	num = libx2_func(num);
	num = mod11_func(num);
	num = mod22_func(num);
	printf("num %d \n", num);
	
	return 0;
}