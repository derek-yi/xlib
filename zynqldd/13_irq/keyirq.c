/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : keyirq.c
 作者      : 邓涛
 版本      : V1.0
 描述      : GPIO按键中断驱动实验
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
#include <linux/of_irq.h>
#include <linux/irq.h>

#define KEY_CNT		1		/* 设备号个数 */
#define KEY_NAME	"key"	/* 名字 */

/* 定义按键状态 */
enum key_status {
	KEY_PRESS = 0,	// 按键按下
	KEY_RELEASE,	// 按键松开
	KEY_KEEP,		// 按键状态保持
};

/* 按键设备结构体 */
struct key_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev结构体 */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int key_gpio;			/* GPIO编号 */
	int irq_num;			/* 中断号 */
	struct timer_list timer;/* 定时器 */
	spinlock_t spinlock;	/* 自旋锁 */
};

static struct key_dev key;				/* 按键设备 */
static int status = KEY_KEEP;	/* 按键状态 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int key_open(struct inode *inode, struct file *filp)
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
static ssize_t key_read(struct file *filp, char __user *buf,
			size_t cnt, loff_t *offt)
{
	unsigned long flags;
	int ret;

	/* 自旋锁上锁 */
	spin_lock_irqsave(&key.spinlock, flags);

	/* 将按键状态信息发送给应用程序 */
	ret = copy_to_user(buf, &status, sizeof(int));

	/* 状态重置 */
	status = KEY_KEEP;

	/* 自旋锁解锁 */
	spin_unlock_irqrestore(&key.spinlock, flags);

	return ret;
}

/*
 * @description		: 向设备写数据 
 * @param - filp	: 设备文件，表示打开的文件描述符
 * @param - buf		: 要写给设备写入的数据
 * @param - cnt		: 要写入的数据长度
 * @param - offt	: 相对于文件首地址的偏移
 * @return			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t key_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param – filp	: 要关闭的设备文件(文件描述符)
 * @return			: 0 成功;其他 失败
 */
static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static void key_timer_function(unsigned long arg)
{
	static int last_val = 1;
	unsigned long flags;
	int current_val;

	/* 自旋锁上锁 */
	spin_lock_irqsave(&key.spinlock, flags);

	/* 读取按键值并判断按键当前状态 */
	current_val = gpio_get_value(key.key_gpio);
	if (0 == current_val && last_val)	// 按下
		status = KEY_PRESS;
	else if (1 == current_val && !last_val)
		status = KEY_RELEASE;	// 松开
	else
		status = KEY_KEEP;		// 状态保持

	last_val = current_val;

	/* 自旋锁解锁 */
	spin_unlock_irqrestore(&key.spinlock, flags);
}

static irqreturn_t key_interrupt(int irq, void *dev_id)
{
	/* 按键防抖处理，开启定时器延时15ms */
	mod_timer(&key.timer, jiffies + msecs_to_jiffies(15));
	return IRQ_HANDLED;
}

static int key_parse_dt(void)
{
	struct device_node *nd;
	const char *str;
	int ret;

	/* 获取key节点 */
	nd = of_find_node_by_path("/key");
	if(NULL == nd) {
		printk(KERN_ERR "key: Failed to get key node\n");
		return -EINVAL;
	}

	/* 读取status属性 */
	ret = of_property_read_string(nd, "status", &str);
	if(!ret) {
		if (strcmp(str, "okay"))
			return -EINVAL;
	}

	/* 获取compatible属性值并进行匹配 */
	ret = of_property_read_string(nd, "compatible", &str);
	if(ret)
		return ret;

	if (strcmp(str, "alientek,key")) {
		printk(KERN_ERR "key: Compatible match failed\n");
		return -EINVAL;
	}

	/* 获取设备树中的key-gpio属性，得到按键的GPIO编号 */
	key.key_gpio = of_get_named_gpio(nd, "key-gpio", 0);
	if(!gpio_is_valid(key.key_gpio)) {
		printk(KERN_ERR "key: Failed to get key-gpio\n");
		return -EINVAL;
	}

	/* 获取GPIO对应的中断号 */
	key.irq_num = irq_of_parse_and_map(nd, 0);
	if (!key.irq_num)
		return -EINVAL;

	return 0;
}

static int key_gpio_init(void)
{
	unsigned long irq_flags;
	int ret;

	/* 申请使用GPIO */
	ret = gpio_request(key.key_gpio, "Key Gpio");
	if (ret)
		return ret;

	/* 将GPIO设置为输入模式 */
	gpio_direction_input(key.key_gpio);

	/* 获取设备树中指定的中断触发类型 */
	irq_flags = irq_get_trigger_type(key.irq_num);
	if (IRQF_TRIGGER_NONE == irq_flags)
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;

	/* 申请中断 */
	ret = request_irq(key.irq_num, key_interrupt, irq_flags, "PS Key0 IRQ", NULL);
	if (ret) {
		gpio_free(key.key_gpio);
		return ret;
	}

	return 0;
}

/* 设备操作函数 */
static struct file_operations key_fops = {
	.owner		= THIS_MODULE,
	.open		= key_open,
	.read		= key_read,
	.write		= key_write,
	.release	= key_release,
};

static int __init mykey_init(void)
{
	int ret;

	/* 初始化自旋锁 */
	spin_lock_init(&key.spinlock);

	/* 设备树解析 */
	ret = key_parse_dt();
	if (ret)
		return ret;

	/* GPIO、中断初始化 */
	ret = key_gpio_init();
	if (ret)
		return ret;

	/* 初始化cdev */
	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);

	/* 添加cdev */
	ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
	if (ret)
		goto out1;

	ret = cdev_add(&key.cdev, key.devid, KEY_CNT);
	if (ret)
		goto out2;

	/* 创建类 */
	key.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(key.class)) {
		ret = PTR_ERR(key.class);
		goto out3;
	}

	/* 创建设备 */
	key.device = device_create(key.class, NULL,
				key.devid, NULL, KEY_NAME);
	if (IS_ERR(key.device)) {
		ret = PTR_ERR(key.device);
		goto out4;
	}

	/* 初始化定时器 */
	init_timer(&key.timer);
	key.timer.function = key_timer_function;

	return 0;

out4:
	class_destroy(key.class);

out3:
	cdev_del(&key.cdev);

out2:
	unregister_chrdev_region(key.devid, KEY_CNT);

out1:
	free_irq(key.irq_num, NULL);
	gpio_free(key.key_gpio);

	return ret;
}

static void __exit mykey_exit(void)
{
	/* 删除定时器 */
	del_timer_sync(&key.timer);

	/* 注销设备 */
	device_destroy(key.class, key.devid);

	/* 注销类 */
	class_destroy(key.class);

	/* 删除cdev */
	cdev_del(&key.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(key.devid, KEY_CNT);

	/* 释放中断 */
	free_irq(key.irq_num, NULL);

	/* 释放GPIO */
	gpio_free(key.key_gpio);
}

/* 驱动模块入口和出口函数注册 */
module_init(mykey_init);
module_exit(mykey_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Gpio Key Interrupt Driver");
MODULE_LICENSE("GPL");
