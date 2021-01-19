/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : beeper.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 领航者开发板蜂鸣器驱动文件。
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#define BEEPER_CNT		1			/* 设备号个数 */
#define BEEPER_NAME		"beeper"	/* 名字 */

/* dtsled设备结构体 */
struct beeper_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int major;				/* 主设备号 */
	int minor;				/* 次设备号 */
	struct device_node *nd;	/* 设备节点 */
	int gpio;				/* LED所使用的GPIO编号 */
};

static struct beeper_dev beeper;	/* led设备 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int beeper_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp	: 要打开的设备文件(文件描述符)
 * @param - buf		: 返回给用户空间的数据缓冲区
 * @param - cnt		: 要读取的数据长度
 * @param - offt	: 相对于文件首地址的偏移
 * @return			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t beeper_read(struct file *filp, char __user *buf,
			size_t cnt, loff_t *offt)
{
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp	: 设备文件，表示打开的文件描述符
 * @param - buf		: 要写给设备写入的数据
 * @param - cnt		: 要写入的数据长度
 * @param - offt	: 相对于文件首地址的偏移
 * @return			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t beeper_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	int ret;
	char kern_buf[1];

	ret = copy_from_user(kern_buf, buf, cnt);	// 得到应用层传递过来的数据
	if(0 > ret) {
		printk(KERN_ERR "kernel write failed!\r\n");
		return -EFAULT;
	}

	if (0 == kern_buf[0])
		gpio_set_value(beeper.gpio, 0);	// 如果传递过来的数据是0则关闭led
	else if (1 == kern_buf[0])
		gpio_set_value(beeper.gpio, 1);	// 如果传递过来的数据是1则点亮led

	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param – filp	: 要关闭的设备文件(文件描述符)
 * @return			: 0 成功;其他 失败
 */
static int beeper_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations beeper_fops = {
	.owner		= THIS_MODULE,
	.open		= beeper_open,
	.read		= beeper_read,
	.write		= beeper_write,
	.release	= beeper_release,
};

static int __init beeper_init(void)
{
	const char *str;
	int ret;

	/* 1.获取led设备节点 */
	beeper.nd = of_find_node_by_path("/beeper");
	if(NULL == beeper.nd) {
		printk(KERN_ERR "beeper: Failed to get beeper node\n");
		return -EINVAL;
	}

	/* 2.读取status属性 */
	ret = of_property_read_string(beeper.nd, "status", &str);
	if(!ret) {
		if (strcmp(str, "okay"))
			return -EINVAL;
	}

	/* 2、获取compatible属性值并进行匹配 */
	ret = of_property_read_string(beeper.nd, "compatible", &str);
	if(0 > ret) {
		printk(KERN_ERR "beeper: Failed to get compatible property\n");
		return ret;
	}

	if (strcmp(str, "alientek,beeper")) {
		printk(KERN_ERR "beeper: Compatible match failed\n");
		return -EINVAL;
	}

	printk(KERN_INFO "beeper: device matching successful!\r\n");

	/* 4.获取设备树中的beeper-gpio属性，得到蜂鸣器所使用的GPIO编号 */
	beeper.gpio = of_get_named_gpio(beeper.nd, "beeper-gpio", 0);
	if(!gpio_is_valid(beeper.gpio)) {
		printk(KERN_ERR "beeper: Failed to get beeper-gpio\n");
		return -EINVAL;
	}

	printk(KERN_INFO "beeper: beeper-gpio num = %d\r\n", beeper.gpio);

	/* 5.向gpio子系统申请使用GPIO */
	ret = gpio_request(beeper.gpio, "Beeper gpio");
	if (ret) {
		printk(KERN_ERR "beeper: Failed to request gpio num %d\n", beeper.gpio);
		return ret;
	}

	/* 6.将gpio管脚设置为输出模式 */
	gpio_direction_output(beeper.gpio, 0);

	/* 7.设置蜂鸣器的初始状态 */
	ret = of_property_read_string(beeper.nd, "default-state", &str);
	if(!ret) {
		if (!strcmp(str, "on"))
			gpio_set_value(beeper.gpio, 1);
		else
			gpio_set_value(beeper.gpio, 0);
	} else
		gpio_set_value(beeper.gpio, 0);

	/* 8.注册字符设备驱动 */
	 /* 创建设备号 */
	if (beeper.major) {
		beeper.devid = MKDEV(beeper.major, 0);
		ret = register_chrdev_region(beeper.devid, BEEPER_CNT, BEEPER_NAME);
		if (ret)
			goto out1;
	} else {
		ret = alloc_chrdev_region(&beeper.devid, 0, BEEPER_CNT, BEEPER_NAME);
		if (ret)
			goto out1;

		beeper.major = MAJOR(beeper.devid);
		beeper.minor = MINOR(beeper.devid);
	}

	printk("beeper: major=%d,minor=%d\r\n", beeper.major, beeper.minor); 

	 /* 初始化cdev */
	beeper.cdev.owner = THIS_MODULE;
	cdev_init(&beeper.cdev, &beeper_fops);

	 /* 添加一个cdev */
	ret = cdev_add(&beeper.cdev, beeper.devid, BEEPER_CNT);
	if (ret)
		goto out2;

	 /* 创建类 */
	beeper.class = class_create(THIS_MODULE, BEEPER_NAME);
	if (IS_ERR(beeper.class)) {
		ret = PTR_ERR(beeper.class);
		goto out3;
	}

	 /* 创建设备 */
	beeper.device = device_create(beeper.class, NULL,
				beeper.devid, NULL, BEEPER_NAME);
	if (IS_ERR(beeper.device)) {
		ret = PTR_ERR(beeper.device);
		goto out4;
	}

	return 0;

out4:
	class_destroy(beeper.class);

out3:
	cdev_del(&beeper.cdev);

out2:
	unregister_chrdev_region(beeper.devid, BEEPER_CNT);

out1:
	gpio_free(beeper.gpio);

	return ret;
}

static void __exit beeper_exit(void)
{
	/* 注销设备 */
	device_destroy(beeper.class, beeper.devid);

	/* 注销类 */
	class_destroy(beeper.class);

	/* 删除cdev */
	cdev_del(&beeper.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(beeper.devid, BEEPER_CNT);

	/* 释放GPIO */
	gpio_free(beeper.gpio);
}

/* 驱动模块入口和出口函数注册 */
module_init(beeper_init);
module_exit(beeper_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Alientek ZYNQ GPIO Beeper Driver");
MODULE_LICENSE("GPL");
