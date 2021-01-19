/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名 		: keyinputApp.c
 作者			: 邓涛
 版本			: V1.0
 描述			: input子系统驱动框架之按键驱动应用层测试代码
 其他			: 无
 使用方法		: ./keyinputApp /dev/input/eventX
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
#include <linux/input.h>

/*
 * @description		: main主程序
 * @param - argc	: argv数组元素个数
 * @param - argv	: 具体参数
 * @return			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret;
	struct input_event ev;

	if(2 != argc) {
		printf("Usage:\n"
		"\t./keyinputApp /dev/input/eventX		@ Open Key\n"
		);
		return -1;
	}

	/* 打开设备 */
	fd = open(argv[1], O_RDWR);
	if(0 > fd) {
		printf("Error: file %s open failed!\r\n", argv[1]);
		return -1;
	}

	/* 读取按键数据 */
	for ( ; ; ) {

		ret = read(fd, &ev, sizeof(struct input_event));
		if (ret) {
			switch (ev.type) {
			case EV_KEY:
				if (KEY_0 == ev.code) {
					if (ev.value)
						printf("PS Key0 Press\n");
					else
						printf("PS Key0 Release\n");
				}
				break;

			/* 其他类型的事件，自行处理 */
			case EV_REL:
				break;
			case EV_ABS:
				break;
			case EV_MSC:
				break;
			case EV_SW:
				break;
			};
		}
		else {
			printf("Error: file %s read failed!\r\n", argv[1]);
			goto out;
		}
	}

out:
	/* 关闭设备 */
	close(fd);
	return 0;
}
