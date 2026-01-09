#include <stdio.h>
#include <string.h>

int func(char *str)
{
	if (str && str[0] == 0) str = NULL;
	if (str == NULL) {
		printf("null str %d \r\n", NULL);
		return 0;
	}
	
	printf("0x%x\r\n", str[0]);
	return 0;
}


int main()
{
	char buff[100];
	int ret;
	char *p = "ABCD";
	
	ret = strncasecmp(p, NULL, 4);
	printf("ret %d \r\n", ret);
	
	memset(buff, 0, sizeof(buff));
	func(buff);
	func(NULL);
	
	return 0;
}