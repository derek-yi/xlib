
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
 
#define NODE_NAME ("/sys/kernel/debug/my_dev/node")

#define TO_WRITE "1234"

#define BUF_LENGTH 128
 
int main()
{
	int filp;
	int res = 0;
	char buf[BUF_LENGTH];
	
	memset(buf, 0, BUF_LENGTH);	
	
	filp = open(NODE_NAME, O_RDWR);
	
	if(filp > 0)
	{
		if( (res = read(filp, buf, BUF_LENGTH) ) < 0)
			printf("read error \n");
		else
			printf("first read is %s\n", buf);
	
		if((res = write(filp, TO_WRITE, strlen(TO_WRITE))) != strlen(TO_WRITE))
			printf("write error \n");
		
		close(filp);
		filp = open(NODE_NAME, O_RDWR);
		memset(buf, 0, BUF_LENGTH);	
	
		if( (res = read(filp, buf, BUF_LENGTH) ) < 0)
			printf("read error \n");
		else
			printf("second read is %s\n", buf);
	}
		
	close(filp);	
	exit(0);
}

