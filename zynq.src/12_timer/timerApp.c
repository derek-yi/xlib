/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名 		: timerApp.c
 作者			: 邓涛
 版本			: V1.0
 描述			: linux内核定时器测试程序
 其他			: 无
 使用方法		: ./timerApp /dev/led
 论坛			: www.openedv.com
 日志			: 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/* ioctl命令 */
#define CMD_LED_CLOSE		(_IO(0XEF, 0x1))	/* 关闭LED */
#define CMD_LED_OPEN		(_IO(0XEF, 0x2))	/* 打开LED */
#define CMD_SET_PERIOD		(_IO(0XEF, 0x3))	/* 设置LED闪烁频率 */

/*
 * @description		: main主程序
 * @param - argc	: argv数组元素个数
 * @param - argv	: 具体参数
 * @return			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret;
	unsigned int cmd;
	unsigned int period;

	if(2 != argc) {
		printf("Usage:\n"
		"\t./timerApp /dev/led		@ open LED device\n"
		);
		return -1;
	}

	/* 打开设备 */
	fd = open(argv[1], O_RDWR);
	if(0 > fd) {
		printf("ERROR: %s file open failed!\r\n", argv[1]);
		return -1;
	}

	/* 通过命令控制LED设备 */
	for ( ; ; ) {

		printf("Input CMD:");
		scanf("%d", &cmd);

		switch (cmd) {

		case 0:
			cmd = CMD_LED_CLOSE;
			break;

		case 1:
			cmd = CMD_LED_OPEN;
			break;

		case 2:
			cmd = CMD_SET_PERIOD;
			printf("Input Timer Period:");
			scanf("%d", &period);
			break;

		case 3:
			close(fd);
			return 0;

		default: break;
		}

		ioctl(fd, cmd, period);
	}
}
