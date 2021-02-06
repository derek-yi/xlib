/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名 		: atomicApp.c
 作者			: 邓涛
 版本			: V1.0
 描述			: 原子变量测试APP，测试原子变量能不能实现一次
				  只允许一个应用程序使用LED。
 其他			: 无
 使用方法		: ./atomicApp /dev/gpioled  0 关闭LED灯
                  ./atomicApp /dev/gpioled  1 打开LED灯
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

/*
 * @description		: main主程序
 * @param - argc	: argv数组元素个数
 * @param - argv	: 具体参数
 * @return			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret;
	int cnt = 0;
	unsigned char buf[1];

	if(3 != argc) {
		printf("Usage:\n"
		"\t./atomicApp /dev/gpioled 1		@ close led\n"
		"\t./atomicApp /dev/gpioled 0		@ open led\n"
		);
		return -1;
	}

	/* 打开设备 */
	fd = open(argv[1], O_RDWR);
	if(0 > fd) {
		printf("ERROR: file %s open failed!\r\n", argv[1]);
		return -1;
	}

	/* 将字符串转换为int型数据 */
	buf[0] = atoi(argv[2]);

	/* 向驱动写入数据 */
	ret = write(fd, buf, sizeof(buf));
	if(0 > ret){
		printf("ERROR: LED Control Failed!\r\n");
		close(fd);
		return -1;
	}

	/* 模拟占用25秒LED设备 */
	for ( ; ; ) {
		sleep(5);
		cnt++;
		printf("App running times:%d\r\n", cnt);
		if(cnt >= 5) break;
	}

	printf("App running finished!\n");

	/* 关闭设备 */
	close(fd);
	return 0;
}
