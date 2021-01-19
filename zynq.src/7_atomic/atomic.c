/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : atomic.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 原子操作实验，使用原子变量来实现对实现设备的互斥访问
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

#define GPIOLED_CNT		1				/* 设备号个数 */
#define GPIOLED_NAME	"gpioled"		/* 名字 */

/* dtsled设备结构体 */
struct gpioled_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int major;				/* 主设备号 */
	int minor;				/* 次设备号 */
	struct device_node *nd;	/* 设备节点 */
	int led_gpio;			/* LED所使用的GPIO编号 */
	atomic_t lock;			/* 原子变量 */
};

static struct gpioled_dev gpioled;	/* led设备 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	/* 通过判断原子变量的值来检查LED有没有被别的应用使用 */
	if (!atomic_dec_and_test(&gpioled.lock)) {
		printk(KERN_ERR "gpioled: Device is busy!\n");
		atomic_inc(&gpioled.lock);	/* 小于0的话就加1,使其原子变量等于0 */
		return -EBUSY;				/* LED被其他应用使用，返回忙 */
	}

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
static ssize_t led_read(struct file *filp, char __user *buf,
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
static ssize_t led_write(struct file *filp, const char __user *buf,
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
		gpio_set_value(gpioled.led_gpio, 0);	// 如果传递过来的数据是0则关闭led
	else if (1 == kern_buf[0])
		gpio_set_value(gpioled.led_gpio, 1);	// 如果传递过来的数据是1则点亮led

	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param – filp	: 要关闭的设备文件(文件描述符)
 * @return			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
	/* 关闭驱动文件的时候释放原子变量 */
	atomic_inc(&gpioled.lock);

	return 0;
}

/* 设备操作函数 */
static struct file_operations gpioled_fops = {
	.owner		= THIS_MODULE,
	.open		= led_open,
	.read		= led_read,
	.write		= led_write,
	.release	= led_release,
};

static int __init led_init(void)
{
	const char *str;
	int ret;

	/* 1.获取led设备节点 */
	gpioled.nd = of_find_node_by_path("/led");
	if(NULL == gpioled.nd) {
		printk(KERN_ERR "gpioled: Failed to get /led node\n");
		return -EINVAL;
	}

	/* 2.读取status属性 */
	ret = of_property_read_string(gpioled.nd, "status", &str);
	if(!ret) {
		if (strcmp(str, "okay"))
			return -EINVAL;
	}

	/* 2、获取compatible属性值并进行匹配 */
	ret = of_property_read_string(gpioled.nd, "compatible", &str);
	if(0 > ret) {
		printk(KERN_ERR "gpioled: Failed to get compatible property\n");
		return ret;
	}

	if (strcmp(str, "alientek,led")) {
		printk(KERN_ERR "gpioled: Compatible match failed\n");
		return -EINVAL;
	}

	printk(KERN_INFO "gpioled: device matching successful!\r\n");

	/* 4.获取设备树中的led-gpio属性，得到LED所使用的GPIO编号 */
	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
	if(!gpio_is_valid(gpioled.led_gpio)) {
		printk(KERN_ERR "gpioled: Failed to get led-gpio\n");
		return -EINVAL;
	}

	printk(KERN_INFO "gpioled: led-gpio num = %d\r\n", gpioled.led_gpio);

	/* 5.向gpio子系统申请使用GPIO */
	ret = gpio_request(gpioled.led_gpio, "LED-GPIO");
	if (ret) {
		printk(KERN_ERR "gpioled: Failed to request led-gpio\n");
		return ret;
	}

	/* 6.将led gpio管脚设置为输出模式 */
	gpio_direction_output(gpioled.led_gpio, 0);

	/* 7.初始化LED的默认状态 */
	ret = of_property_read_string(gpioled.nd, "default-state", &str);
	if(!ret) {
		if (!strcmp(str, "on"))
			gpio_set_value(gpioled.led_gpio, 1);
		else
			gpio_set_value(gpioled.led_gpio, 0);
	} else
		gpio_set_value(gpioled.led_gpio, 0);

	/* 8.注册字符设备驱动 */
	 /* 创建设备号 */
	if (gpioled.major) {
		gpioled.devid = MKDEV(gpioled.major, 0);
		ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
		if (ret)
			goto out1;
	} else {
		ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		if (ret)
			goto out1;

		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.devid);
	}

	printk("gpioled: major=%d,minor=%d\r\n",gpioled.major, gpioled.minor); 

	 /* 初始化cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);

	 /* 添加一个cdev */
	ret = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
	if (ret)
		goto out2;

	 /* 创建类 */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class)) {
		ret = PTR_ERR(gpioled.class);
		goto out3;
	}

	 /* 创建设备 */
	gpioled.device = device_create(gpioled.class, NULL,
				gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device)) {
		ret = PTR_ERR(gpioled.device);
		goto out4;
	}

	/* 9.初始化原子变量 */
	atomic_set(&gpioled.lock, 1);	/* 原子变量初始值为1 */

	return 0;

out4:
	class_destroy(gpioled.class);

out3:
	cdev_del(&gpioled.cdev);

out2:
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

out1:
	gpio_free(gpioled.led_gpio);

	return ret;
}

static void __exit led_exit(void)
{
	/* 注销设备 */
	device_destroy(gpioled.class, gpioled.devid);

	/* 注销类 */
	class_destroy(gpioled.class);

	/* 删除cdev */
	cdev_del(&gpioled.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

	/* 释放GPIO */
	gpio_free(gpioled.led_gpio);
}

/* 驱动模块入口和出口函数注册 */
module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Alientek ZYNQ GPIO LED Driver");
MODULE_LICENSE("GPL");
