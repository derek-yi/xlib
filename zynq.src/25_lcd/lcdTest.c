/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : lcdTest.c
 作者      : 邓涛
 版本      : V1.0
 描述      : LCD应用层测试程序
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2020/7/23 邓涛创建
 ***************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

static void display_demo_1 (unsigned char *frame, unsigned int width, unsigned int height, unsigned int stride)
{
	unsigned int xcoi, ycoi;
	unsigned char wRed, wBlue, wGreen;
	unsigned int iPixelAddr = 0;

	for(ycoi = 0; ycoi < height; ycoi++) {

		for(xcoi = 0; xcoi < (width * 3); xcoi += 3) {

			if (((xcoi / 4) & 0x20) ^ (ycoi & 0x20)) {
				wRed = 255;
				wGreen = 255;
				wBlue = 255;
			} else {
				wRed = 0;
				wGreen = 0;
				wBlue = 0;
			}

			frame[xcoi + iPixelAddr + 0] = wRed;
			frame[xcoi + iPixelAddr + 1] = wGreen;
			frame[xcoi + iPixelAddr + 2] = wBlue;
		}

		iPixelAddr += stride;
	}
}

static void display_demo_2 (unsigned char *frame, unsigned int width, unsigned int height, unsigned int stride)
{
	unsigned int xcoi, ycoi;
	unsigned int iPixelAddr = 0;
	unsigned char wRed, wBlue, wGreen;
	unsigned int xInt;

	xInt = width * 3 / 8;
	for(ycoi = 0; ycoi < height; ycoi++) {

		for(xcoi = 0; xcoi < (width*3); xcoi+=3) {

			if (xcoi < xInt) {                                   //White color
				wRed = 255;
				wGreen = 255;
				wBlue = 255;
			}
			else if ((xcoi >= xInt) && (xcoi < xInt*2)) {         //YELLOW color
				wRed = 255;
				wGreen = 255;
				wBlue = 0;
			}
			else if ((xcoi >= xInt * 2) && (xcoi < xInt * 3)) {        //CYAN color
				wRed = 0;
				wGreen = 255;
				wBlue = 255;
			}
			else if ((xcoi >= xInt * 3) && (xcoi < xInt * 4)) {        //GREEN color
				wRed = 0;
				wGreen = 255;
				wBlue = 0;
			}
			else if ((xcoi >= xInt * 4) && (xcoi < xInt * 5)) {        //MAGENTA color
				wRed = 255;
				wGreen = 0;
				wBlue = 255;
			}
			else if ((xcoi >= xInt * 5) && (xcoi < xInt * 6)) {        //RED color
				wRed = 255;
				wGreen = 0;
				wBlue = 0;
			}
			else if ((xcoi >= xInt * 6) && (xcoi < xInt * 7)) {        //BLUE color
				wRed = 0;
				wGreen = 0;
				wBlue = 255;
			}
			else {                                                //BLACK color
				wRed = 0;
				wGreen = 0;
				wBlue = 0;
			}

			frame[xcoi+iPixelAddr + 0] = wRed;
			frame[xcoi+iPixelAddr + 1] = wGreen;
			frame[xcoi+iPixelAddr + 2] = wBlue;
		}

		iPixelAddr += stride;
	}
}

int main (int argc, char **argv)
{
	struct fb_var_screeninfo fb_var = {0};
	struct fb_fix_screeninfo fb_fix = {0};
	unsigned int screensize;
	unsigned char *base;
	int fd;

	/* 打开LCD */
	fd = open("/dev/fb0", O_RDWR);

	if (fd < 0) {
		printf("Error: Failed to open /dev/fb0 device.\n");
		return fd;
	}

	/* 获取framebuffer设备的参数信息 */
	ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
	ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);

	/* mmap映射 */
	screensize = fb_var.yres * fb_fix.line_length;
	base = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((unsigned char *)-1 == base) {
		close(fd);
		return -1;
	}

	memset(base, 0x00, screensize);		// 显存清零

	/* 循环显示不同颜色 */
	for ( ; ; ) {

		display_demo_1(base, fb_var.xres, fb_var.yres, fb_fix.line_length);
		sleep(2);

		display_demo_2(base, fb_var.xres, fb_var.yres, fb_fix.line_length);
		sleep(2);
	}

	/* 关闭设备 释放内存 */
	memset(base, 0x00, screensize);
	munmap(base, screensize);
	close(fd);
	return 0;
}

