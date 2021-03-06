/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名     : chrdevbaseApp.c
 作者       : 正点原子
 版本       : V1.0
 描述       : chrdevbase驱测试APP。
 其他       : 使用方法：./chrdevbaseApp /dev/chrdevbase <1>|<2>
              argv[2] 1:读文件
              argv[2] 2:写文件
 论坛       : www.openedv.com
 日志       : 初版 V1.0 2019/1/30 正点原子创建
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static char usrdata[] = {"usr data!"};

/*
 * @description		: main主程序
 * @param - argc	: argv数组元素个数
 * @param - argv	: 具体参数
 * @return			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	char readbuf[100], writebuf[100];

	if(argc != 3){
		printf("Usage: %s <file> <1|2>\r\n", argv[0]);
		return -1;
	}

	filename = argv[1];

	/* 打开驱动文件 */
	fd  = open(filename, O_RDWR);
	if(fd < 0){
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	if(atoi(argv[2]) == 1){ /* 从驱动文件读取数据 */
		retvalue = read(fd, readbuf, 50);
		if(retvalue < 0){
			printf("read file %s failed!\r\n", filename);
		}else{
			/*  读取成功，打印出读取成功的数据 */
			printf("read data:%s\r\n",readbuf);
		}
	}

	if(atoi(argv[2]) == 2){
		/* 向设备驱动写数据 */
		memcpy(writebuf, usrdata, sizeof(usrdata));
		retvalue = write(fd, writebuf, 50);
		if(retvalue < 0){
			printf("write file %s failed!\r\n", filename);
		}
	}

	/* 关闭设备 */
	retvalue = close(fd);
	if(retvalue < 0){
		printf("Can't close file %s\r\n", filename);
		return -1;
	}

	return 0;
}

/*
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# cat /proc/devices | grep 'chr'
200 chrdevbase
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# mknod /dev/chrdevbase c 200 0
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# ./a.out /dev/chrdevbase 1
read data:kernel data!
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# ./a.out /dev/chrdevbase 2
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# ./a.out /dev/chrdevbase 1
read data:kernel data!
root@ubox:~/github/xlib/zynqldd/1_chrdevbase# dmesg
[  682.026470] dtsled: loading out-of-tree module taints kernel.
[  682.028994] led node can not found!
[  839.796867] chrdevbase_init()
[ 1850.124796] kernel senddata ok!
[ 1854.748004] kernel recevdata:usr data!
[ 1856.199053] kernel senddata ok!
*/
