/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : leddevice.c
 作者      : 邓涛
 版本      : V1.0
 描述      : platform总线编程示例之platform设备模块
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * GPIO相关寄存器地址定义
 */
#define DATA_REG			0xE000A040
#define DIRM_REG			0xE000A204
#define OUTEN_REG			0xE000A208
#define INTDIS_REG			0xE000A214
#define APER_CLK_CTRL_REG	0xF800012C

/*
 * @description		: platform设备模块卸载时此函数会执行
 * @param - dev		: 要释放的设备
 * @return			: 无
 */
static void myled_release(struct device *dev)
{
	printk(KERN_INFO "myled: led platform device release!\r\n"); 
}

/*
 * platform设备资源信息
 * 也就是PS_LED0所使用的所有寄存器资源
 */
static struct resource myled_resources[] = {
	[0] = {
		.start	= DATA_REG,
		.end	= DATA_REG + 3,
		.flags	= IORESOURCE_MEM,
	},  
	[1] = {
		.start	= DIRM_REG,
		.end	= DIRM_REG + 3,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= OUTEN_REG,
		.end	= OUTEN_REG + 3,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= INTDIS_REG,
		.end	= INTDIS_REG + 3,
		.flags	= IORESOURCE_MEM,
	},
	[4] = {
		.start	= APER_CLK_CTRL_REG,
		.end	= APER_CLK_CTRL_REG + 3,
		.flags	= IORESOURCE_MEM,
	},
};

/*
 * platform设备结构体
 */
static struct platform_device myled_device = {
	.name = "zynq-led",
	.id = -1,
	.dev = {
		.release = &myled_release,
	},
	.num_resources = ARRAY_SIZE(myled_resources),
	.resource = myled_resources,
};

/*
 * @description 	: 模块入口函数 
 * @param       	: 无
 * @return      	: 无
 */
static int __init myled_device_init(void)
{
	return platform_device_register(&myled_device);
}

/*
 * @description		: 模块出口函数
 * @param			: 无
 * @return			: 无
 */
static void __exit myled_device_exit(void)
{
	platform_device_unregister(&myled_device);
}

module_init(myled_device_init);
module_exit(myled_device_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Led Platform Device");
MODULE_LICENSE("GPL");
