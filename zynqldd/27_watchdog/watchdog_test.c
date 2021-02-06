#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/watchdog.h>

int main(int argc, char *argv[])
{
	int fd = -1;
	int cmd = -1;

	/* 打开看门狗 */
	fd = open("/dev/watchdog0", O_RDWR);
	if(fd < 0) {
		printf("Error: Failed to open /dev/watchdog0\n");
		return -1;
	}

	if (argc < 2) {
		printf("<%s> set <timeout>\n", argv[0]);
		printf("<%s> feed\n", argv[0]);
		goto out;
	}


	if (!strcmp(argv[1], "set"))			//设置看门狗超时时间
		cmd = 0;
	else if (!strcmp(argv[1], "feed"))	//喂狗
		cmd = 1;
	else
		goto out;

	switch (cmd) {
	case 0: {
		int timeout = atoi(argv[2]);
		printf("Timeout=%d\n", timeout);
		ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	}
		break;

	case 1:
		ioctl(fd, WDIOC_KEEPALIVE, NULL);
		break;

	default: break;
	}

out:
	close(fd);
	return 0;
}
