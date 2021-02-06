/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : timer.c
 作者      : 邓涛
 版本      : V1.0
 描述      : linux内核定时器测试
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/types.h>

#define LED_CNT		1			/* 设备号个数 */
#define LED_NAME	"led"		/* 名字 */

/* ioctl函数命令定义 */
#define CMD_LED_CLOSE		(_IO(0XEF, 0x1))	/* 关闭LED */
#define CMD_LED_OPEN		(_IO(0XEF, 0x2))	/* 打开LED */
#define CMD_SET_PERIOD		(_IO(0XEF, 0x3))	/* 设置LED闪烁频率 */


/* led设备结构体 */
struct led_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int major;				/* 主设备号 */
	int minor;				/* 次设备号 */
	struct device_node *nd;	/* 设备节点 */
	int led_gpio;			/* GPIO编号 */
	int period;				/* 定时周期,单位为ms */
	struct timer_list timer;/* 定义一个定时器 */
	spinlock_t spinlock;		/* 定义自旋锁 */
};

static struct led_dev led;	/* led设备 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
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
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param – filp	: 要关闭的设备文件(文件描述符)
 * @return			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * @description		: ioctl函数，
 * @param – filp	: 要打开的设备文件(文件描述符)
 * @param - cmd		: 应用程序发送过来的命令
 * @param - arg		: 参数
 * @return			: 0 成功;其他 失败
 */
static long led_unlocked_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	unsigned long flags;

	/* 自旋锁上锁 */
	spin_lock_irqsave(&led.spinlock, flags);

	switch (cmd) {

	case CMD_LED_CLOSE:
		del_timer_sync(&led.timer);
		gpio_set_value(led.led_gpio, 0);
		break;

	case CMD_LED_OPEN:
		del_timer_sync(&led.timer);
		gpio_set_value(led.led_gpio, 1);
		break;

	case CMD_SET_PERIOD:
		led.period = arg;
		mod_timer(&led.timer, jiffies + msecs_to_jiffies(arg));
		break;

	default: break;
	}

	/* 自旋锁解锁 */
	spin_unlock_irqrestore(&led.spinlock, flags);

	return 0;
}

/* 设备操作函数 */
static struct file_operations led_fops = {
	.owner			= THIS_MODULE,
	.open			= led_open,
	.read			= led_read,
	.write			= led_write,
	.release		= led_release,
	.unlocked_ioctl	= led_unlocked_ioctl,
};

/* 定时器回调函数 */
static void led_timer_function(unsigned long arg)
{
	static bool on = 1;
	unsigned long flags;

	/* 每次都取反，实现LED灯反转 */
	on = !on;

	/* 自旋锁上锁 */
	spin_lock_irqsave(&led.spinlock, flags);

	/* 设置GPIO电平状态 */
	gpio_set_value(led.led_gpio, on);

	/* 重启定时器 */
	mod_timer(&led.timer, jiffies + msecs_to_jiffies(led.period));

	/* 自旋锁解锁 */
	spin_unlock_irqrestore(&led.spinlock, flags);
}

static int __init led_init(void)
{
	const char *str;
	int val;
	int ret;

	/* 初始化自旋锁 */
	spin_lock_init(&led.spinlock);

	/* 1.获取led设备节点 */
	led.nd = of_find_node_by_path("/led");
	if(NULL == led.nd) {
		printk(KERN_ERR "led: Failed to get led node\n");
		return -EINVAL;
	}

	/* 2.读取status属性 */
	ret = of_property_read_string(led.nd, "status", &str);
	if(!ret) {
		if (strcmp(str, "okay"))
			return -EINVAL;
	}

	/* 3.获取compatible属性值并进行匹配 */
	ret = of_property_read_string(led.nd, "compatible", &str);
	if(ret) {
		printk(KERN_ERR "led: Failed to get compatible property\n");
		return ret;
	}

	if (strcmp(str, "alientek,led")) {
		printk(KERN_ERR "led: Compatible match failed\n");
		return -EINVAL;
	}

	printk(KERN_INFO "led: device matching successful!\r\n");

	/* 4.获取设备树中的led-gpio属性，得到LED所使用的GPIO编号 */
	led.led_gpio = of_get_named_gpio(led.nd, "led-gpio", 0);
	if(!gpio_is_valid(led.led_gpio)) {
		printk(KERN_ERR "led: Failed to get led-gpio\n");
		return -EINVAL;
	}

	printk(KERN_INFO "led: led-gpio num = %d\r\n", led.led_gpio);

	/* 5.向gpio子系统申请使用GPIO */
	ret = gpio_request(led.led_gpio, "LED Gpio");
	if (ret) {
		printk(KERN_ERR "led: Failed to request led-gpio\n");
		return ret;
	}

	/* 6.设置LED灯初始状态 */
	ret = of_property_read_string(led.nd, "default-state", &str);
	if(!ret) {
		if (!strcmp(str, "on"))
			val = 1;
		else
			val = 0;
	} else
		val = 0;

	gpio_direction_output(led.led_gpio, val);

	/* 7.注册字符设备驱动 */
	 /* 创建设备号 */
	if (led.major) {
		led.devid = MKDEV(led.major, 0);
		ret = register_chrdev_region(led.devid, LED_CNT, LED_NAME);
		if (ret)
			goto out1;
	} else {
		ret = alloc_chrdev_region(&led.devid, 0, LED_CNT, LED_NAME);
		if (ret)
			goto out1;

		led.major = MAJOR(led.devid);
		led.minor = MINOR(led.devid);
	}

	printk(KERN_INFO "led: major=%d, minor=%d\r\n", led.major, led.minor); 

	 /* 初始化cdev */
	led.cdev.owner = THIS_MODULE;
	cdev_init(&led.cdev, &led_fops);

	 /* 添加cdev */
	ret = cdev_add(&led.cdev, led.devid, LED_CNT);
	if (ret)
		goto out2;

	 /* 创建类 */
	led.class = class_create(THIS_MODULE, LED_NAME);
	if (IS_ERR(led.class)) {
		ret = PTR_ERR(led.class);
		goto out3;
	}

	 /* 创建设备 */
	led.device = device_create(led.class, NULL,
				led.devid, NULL, LED_NAME);
	if (IS_ERR(led.device)) {
		ret = PTR_ERR(led.device);
		goto out4;
	}

	/* 8.初始化timer，绑定定时器处理函数，此时还未设置周期，所以不会激活定时器 */
	init_timer(&led.timer);
	led.timer.function = led_timer_function;

	return 0;

out4:
	class_destroy(led.class);

out3:
	cdev_del(&led.cdev);

out2:
	unregister_chrdev_region(led.devid, LED_CNT);

out1:
	gpio_free(led.led_gpio);

	return ret;
}

static void __exit led_exit(void)
{
	/* 删除定时器 */
	del_timer_sync(&led.timer);

	/* 注销设备 */
	device_destroy(led.class, led.devid);

	/* 注销类 */
	class_destroy(led.class);

	/* 删除cdev */
	cdev_del(&led.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(led.devid, LED_CNT);

	/* 释放GPIO */
	gpio_free(led.led_gpio);
}

/* 驱动模块入口和出口函数注册 */
module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Alientek Gpio LED Driver");
MODULE_LICENSE("GPL");
