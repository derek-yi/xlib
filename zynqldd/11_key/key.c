/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : key.c
 作者      : 邓涛
 版本      : V1.0
 描述      : Linux按键输入驱动实验
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

#define KEY_CNT		1		/* 设备号个数 */
#define KEY_NAME	"key"	/* 名字 */

/* dtsled设备结构体 */
struct key_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int major;				/* 主设备号 */
	int minor;				/* 次设备号 */
	struct device_node *nd;	/* 设备节点 */
	int key_gpio;			/* GPIO编号 */
	int key_val;			/* 按键值 */
	struct mutex mutex;		/* 互斥锁 */
};

static struct key_dev key;	/* led设备 */

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
	int ret = 0;

	/* 互斥锁上锁 */
	if (mutex_lock_interruptible(&key.mutex))
		return -ERESTARTSYS;

	/* 读取按键数据 */
	if (!gpio_get_value(key.key_gpio)) {
		while(!gpio_get_value(key.key_gpio));
		key.key_val = 0x0;
	} else
		key.key_val = 0xFF;

	/* 将按键数据发送给应用程序 */
	ret = copy_to_user(buf, &key.key_val, sizeof(int));

	/* 解锁 */
	mutex_unlock(&key.mutex);

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
	const char *str;
	int ret;

	/* 初始化互斥锁 */
	mutex_init(&key.mutex);

	/* 1.获取key节点 */
	key.nd = of_find_node_by_path("/key");
	if(NULL == key.nd) {
		printk(KERN_ERR "key: Failed to get key node\n");
		return -EINVAL;
	}

	/* 2.读取status属性 */
	ret = of_property_read_string(key.nd, "status", &str);
	if(!ret) {
		if (strcmp(str, "okay"))
			return -EINVAL;
	}

	/* 3.获取compatible属性值并进行匹配 */
	ret = of_property_read_string(key.nd, "compatible", &str);
	if(ret) {
		printk(KERN_ERR "key: Failed to get compatible property\n");
		return ret;
	}

	if (strcmp(str, "alientek,key")) {
		printk(KERN_ERR "key: Compatible match failed\n");
		return -EINVAL;
	}

	printk(KERN_INFO "key: device matching successful!\r\n");

	/* 4.获取设备树中的key-gpio属性，得到按键所使用的GPIO编号 */
	key.key_gpio = of_get_named_gpio(key.nd, "key-gpio", 0);
	if(!gpio_is_valid(key.key_gpio)) {
		printk(KERN_ERR "key: Failed to get key-gpio\n");
		return -EINVAL;
	}

	printk(KERN_INFO "key: key-gpio num = %d\r\n", key.key_gpio);

	/* 5.申请GPIO */
	ret = gpio_request(key.key_gpio, "Key Gpio");
	if (ret) {
		printk(KERN_ERR "key: Failed to request key-gpio\n");
		return ret;
	}

	/* 6.将GPIO设置为输入模式 */
	gpio_direction_input(key.key_gpio);

	/* 7.注册字符设备驱动 */
	 /* 创建设备号 */
	if (key.major) {
		key.devid = MKDEV(key.major, 0);
		ret = register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
		if (ret)
			goto out1;
	} else {
		ret = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
		if (ret)
			goto out1;

		key.major = MAJOR(key.devid);
		key.minor = MINOR(key.devid);
	}

	printk(KERN_INFO "key: major=%d, minor=%d\r\n", key.major, key.minor); 

	 /* 初始化cdev */
	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);

	 /* 添加cdev */
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

	return 0;

out4:
	class_destroy(key.class);

out3:
	cdev_del(&key.cdev);

out2:
	unregister_chrdev_region(key.devid, KEY_CNT);

out1:
	gpio_free(key.key_gpio);

	return ret;
}

static void __exit mykey_exit(void)
{
	/* 注销设备 */
	device_destroy(key.class, key.devid);

	/* 注销类 */
	class_destroy(key.class);

	/* 删除cdev */
	cdev_del(&key.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(key.devid, KEY_CNT);

	/* 释放GPIO */
	gpio_free(key.key_gpio);
}

/* 驱动模块入口和出口函数注册 */
module_init(mykey_init);
module_exit(mykey_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Alientek Gpio Key Driver");
MODULE_LICENSE("GPL");
