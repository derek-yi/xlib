/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名 		: pcf8563App.c
 作者			: 邓涛
 版本			: V1.0
 描述			: Linux I2C子系统框架编程示例之rtc pcf8563驱动应用层测试代码
 其他			: 无
 使用方法		: ./pcf8563App /dev/pcf8563 read
				  ./pcf8563App /dev/pcf8563 write
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

struct pcf8563_time {
	int sec;
	int min;
	int hour;
	int day;
	int wday;
	int mon;
	int year;
};

/*
 * @description		: main主程序
 * @param - argc	: argv数组元素个数
 * @param - argv	: 具体参数
 * @return			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret;
	struct pcf8563_time time = {0};

	/* 打开设备 */
	fd = open(argv[1], O_RDWR);
	if(0 > fd) {
		printf("Error: file %s open failed!\n", argv[1]);
		return -1;
	}

	if (!strcmp(argv[2], "read")) {		// 读取时间
		/* 读取RTC */
		ret = read(fd, &time, sizeof(struct pcf8563_time));
		if (0 > ret) {
			printf("Error: file %s read failed!\n", argv[1]);
			goto out;
		}

		printf("%d-%d-%d %d:%d:%d ", time.year, time.mon, time.day,
					time.hour, time.min, time.sec);
		switch(time.wday) {

		case 0:
			printf("星期日\n");
			break;

		case 1:
			printf("星期一\n");
			break;

		case 2:
			printf("星期二\n");
			break;

		case 3:
			printf("星期三\n");
			break;

		case 4:
			printf("星期四\n");
			break;

		case 5:
			printf("星期五\n");
			break;

		case 6:
			printf("星期六\n");
			break;
		}
	}
	else {
		int data;
		printf("年:");
		scanf("%d", &data);
		time.year = data;

		printf("\n月:");
		scanf("%d", &data);
		time.mon = data;

		printf("\n日:");
		scanf("%d", &data);
		time.day = data;

		printf("\n星期:");
		scanf("%d", &data);
		time.wday = data;

		printf("\n时:");
		scanf("%d", &data);
		time.hour = data;

		printf("\n分:");
		scanf("%d", &data);
		time.min = data;

		printf("\n秒:");
		scanf("%d", &data);
		time.sec = data;

		/* 将时间写入RTC */
		write(fd, &time, sizeof(struct pcf8563_time));
	}

out:
	/* 关闭文件 */
	close(fd);
	return ret;
}
