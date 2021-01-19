/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : miscbeep.c
 作者      : 邓涛
 版本      : V1.0
 描述      : misc设备驱动框架编程示例之蜂鸣器驱动
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>


/* 蜂鸣器设备结构体 */
struct mybeep_dev {
	struct miscdevice mdev;		// misc设备
	int gpio;					// gpio编号
};

struct mybeep_dev beep_dev;		// 定义一个蜂鸣器设备

/*
 * @description			: beep相关初始化操作
 * @param - pdev		: struct platform_device指针，也就是platform设备指针
 * @return				: 成功返回0，失败返回负数
 */
static int mybeep_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	/* 从设备树中获取GPIO */
	beep_dev.gpio = of_get_named_gpio(dev->of_node, "beeper-gpio", 0);
	if(!gpio_is_valid(beep_dev.gpio)) {
		dev_err(dev, "Failed to get gpio");
		return -EINVAL;
	}

	/* 申请使用GPIO */
	ret = devm_gpio_request(dev, beep_dev.gpio, "Beep Gpio");
	if (ret) {
		dev_err(dev, "Failed to request gpio");
		return ret;
	}

	/* 将GPIO设置为输出模式并将输出低电平 */
	gpio_direction_output(beep_dev.gpio, 0);

	return 0;
}

/*
 * @description			: 打开设备
 * @param – inode		: 传递给驱动的inode
 * @param - filp		: 设备文件，file结构体有个叫做private_data的成员变量
 * 						  一般在open的时候将private_data指向设备结构体。
 * @return				: 0 成功;其他 失败
 */
static int mybeep_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * @description			: 向设备写数据 
 * @param - filp		: 设备文件，表示打开的文件描述符
 * @param - buf			: 要写给设备写入的数据
 * @param - cnt			: 要写入的数据长度
 * @param - offt		: 相对于文件首地址的偏移
 * @return				: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t mybeep_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	int ret;
	char kern_buf[5];

	ret = copy_from_user(kern_buf, buf, cnt);		// 得到应用层传递过来的数据
	if(0 > ret) {
		printk(KERN_ERR "mybeep: Failed to copy data from user buffer\r\n");
		return -EFAULT;
	}

	if (0 == kern_buf[0])
		gpio_set_value(beep_dev.gpio, 0);			// 如果传递过来的数据是0则关闭beep
	else if (1 == kern_buf[0])
		gpio_set_value(beep_dev.gpio, 1);			// 如果传递过来的数据是1则打开beep

	return 0;
}

/* 蜂鸣器设备操作函数集 */
static struct file_operations mybeep_fops = {
	.owner	= THIS_MODULE,
	.open	= mybeep_open,
	.write	= mybeep_write,
};

/*
 * @description			: platform驱动的probe函数，当驱动与设备
 * 						  匹配成功以后此函数就会执行
 * @param - pdev		: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int mybeep_probe(struct platform_device *pdev)
{
	struct miscdevice *mdev;
	int ret;

	dev_info(&pdev->dev, "BEEP device and driver matched successfully!\n");

	/* 初始化BEEP */
	ret = mybeep_init(pdev);
	if (ret)
		return ret;

	/* 初始化beep_data变量 */
	mdev = &beep_dev.mdev;
	mdev->name	= "zynq-beep";			// 设备名
	mdev->minor	= MISC_DYNAMIC_MINOR;	// 动态分配次设备号
	mdev->fops	= &mybeep_fops;			// 绑定file_operations结构体

	/* 向linux系统misc驱动框架核心层注册一个beep设备 */
	return misc_register(mdev);
}

/*
 * @description			: platform驱动模块卸载时此函数会执行
 * @param - dev			: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int mybeep_remove(struct platform_device *pdev)
{
	/* 注销模块时关闭BEEP */
	gpio_set_value(beep_dev.gpio, 0);

	/* 注销misc设备驱动 */
	misc_deregister(&beep_dev.mdev);

	dev_info(&pdev->dev, "BEEP driver has been removed!\n");
	return 0;
}

/* 匹配列表 */
static const struct of_device_id beep_of_match[] = {
	{ .compatible = "alientek,beeper" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver mybeep_driver = {
	.driver = {
		.name			= "zynq-beep",		// 驱动名字，用于和设备匹配
		.of_match_table	= beep_of_match,	// 设备树匹配表，用于和设备树中定义的设备匹配
	},
	.probe		= mybeep_probe,		// probe函数
	.remove		= mybeep_remove,	// remove函数
};

module_platform_driver(mybeep_driver);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("BEEP Driver Based on Misc Driver Framework");
MODULE_LICENSE("GPL");
