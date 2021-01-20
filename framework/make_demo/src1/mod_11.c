
#include <pub1.h>

#include <mod_11.h>
#include <mod_22.h>

int mod11_func(int param)
{
	return param+1;
}

int main()
{
	int ret;
	
	printf("VERSION_NUM: %d \r\n", VERSION_NUM);

	ret	= mod11_func(1);
	printf("mod11_func: %d \r\n", ret);
	
	ret = mod22_func(2);
	printf("mod22_func: %d \r\n", ret);

	ret = libx1_func(3);
	printf("libx1_func: %d \r\n", ret);
	
	return 0;
}