/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : spitest.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 基于Linux SPI总线框架的虚拟设备驱动示例
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <linux/uaccess.h>

#define DEVICE_NAME		"myspi"	/* 名字 */

/*
 * 自定义结构体myspi_dev
 * 用于描述这个虚拟的spi从机设备信息
 */
struct myspi_dev {
	struct spi_device *spi;			/* spi从机设备对象指针 */
	dev_t devid;					/* 设备号 */
	struct cdev cdev;				/* cdev结构体 */
	struct class *class;			/* 类 */
	struct device *device;			/* 设备 */
};

static struct myspi_dev myspi;	// 定义一个spi虚拟设备

/*
 * @description			: 打开设备
 * @param – inode		: 传递给驱动的inode
 * @param - filp		: 设备文件，file结构体有个叫做private_data的成员变量
 * 						  一般在open的时候将private_data指向设备结构体。
 * @return				: 0 成功;其他 失败
 */
static int myspi_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &myspi;
	return 0;
}

/*
 * @description			: 从设备读取数据 
 * @param – filp		: 要打开的设备文件(文件描述符)
 * @param - buf			: 返回给用户空间的数据缓冲区
 * @param - cnt			: 要读取的数据长度
 * @param – off			: 相对于文件首地址的偏移
 * @return				: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t myspi_read(struct file *filp, char __user *buf,
			size_t cnt, loff_t *off)
{
	struct myspi_dev *dev = filp->private_data;
	struct spi_message msg = {0};
	struct spi_transfer xfers[2] = {0};
	unsigned char tx[5];
	unsigned char rx[5];
	int ret;

	/* 填充两次传输序列 */
	xfers[0].tx_buf = tx;			/* 发送数据缓存区 */
	xfers[0].bits_per_word = 8;		/* 一次传输8个bit */
	xfers[0].len = 1;				/* 传输长度为1个字节 */

	xfers[1].tx_buf = rx;			/* 接收数据缓存区 */
	xfers[1].bits_per_word = 8;		/* 一次传输8个bit */
	xfers[1].len = 1;				/* 传输长度为1个字节 */

	tx[0] = 0x45;			/* 要读取的从机设备寄存器地址（举个例子，假设寄存器地址是一个字节） */
	spi_message_init(&msg);	/* 初始化spi_message */
	spi_message_add_tail(&xfers[0], &msg);	/* 将发送序列添加到spi_message */
	spi_message_add_tail(&xfers[1], &msg);	/* 将读取序列添加到spi_message */
	ret = spi_sync(dev->spi, &msg);			/* 执行同步数据传输 */
	if (ret)
		return ret;

	/* 将数据拷贝到用户空间 */
	return copy_to_user(buf, rx, 1);
}

/*
 * @description			: 向设备写数据 
 * @param – filp		: 设备文件，表示打开的文件描述符
 * @param - buf			: 要写给设备写入的数据
 * @param - cnt			: 要写入的数据长度
 * @param - offt		: 相对于文件首地址的偏移
 * @return				: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t myspi_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	struct myspi_dev *dev = filp->private_data;
	struct spi_message msg = {0};
	struct spi_transfer xfer = {0};
	unsigned char tx[5];
	int ret;

	xfer.tx_buf = tx;		/* 发送数据缓存区 */
	xfer.bits_per_word = 8;	/* 一次传输8个bit */
	xfer.len = 2;			/* 传输长度为2个字节：寄存器地址+写入值 */

	tx[0] = 0x55;			/* 要写入的从机设备寄存器地址（举个例子，假设寄存器地址是一个字节） */
	tx[1] = 0xFF;			/* 要往该寄存器写入的数据 */
	spi_message_init(&msg);	/* 初始化spi_message */
	spi_message_add_tail(&xfer, &msg);	/* 将spi_transfer添加到spi_message */
	ret = spi_sync(dev->spi, &msg);		/* 执行同步数据传输 */
	if (ret)
		return ret;

	return cnt;
}

/*
 * @description			: 关闭/释放设备
 * @param - filp		: 要关闭的设备文件(文件描述符)
 * @return				: 0 成功;其他 失败
 */
static int myspi_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * file_operations结构体变量
 */
static const struct file_operations myspi_ops = {
	.owner = THIS_MODULE,
	.open = myspi_open,
	.read = myspi_read,
	.write = myspi_write,
	.release = myspi_release,
};

static int myspi_init(struct myspi_dev *dev)
{
	/* 对设备进行初始化操作 */
	/* 在这个函数中对SPI从机设备进行相关初始化操作 */
	/* ...... */
	return 0;
}

static int myspi_probe(struct spi_device *spi)
{
	int ret;

	/* 初始化虚拟设备 */
	myspi.spi = spi;
	ret = myspi_init(&myspi);
	if (ret)
		return ret;

	/* 申请设备号 */
	ret = alloc_chrdev_region(&myspi.devid, 0, 1, DEVICE_NAME);
	if (ret)
		return ret;

	/* 初始化字符设备cdev */
	myspi.cdev.owner = THIS_MODULE;
	cdev_init(&myspi.cdev, &myspi_ops);

	/* 添加cdev */
	ret = cdev_add(&myspi.cdev, myspi.devid, 1);
	if (ret)
		goto out1;

	/* 创建类class */
	myspi.class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(myspi.class)) {
		ret = PTR_ERR(myspi.class);
		goto out2;
	}

	/* 创建设备 */
	myspi.device = device_create(myspi.class, &spi->dev,
				myspi.devid, NULL, DEVICE_NAME);
	if (IS_ERR(myspi.device)) {
		ret = PTR_ERR(myspi.device);
		goto out3;
	}

	spi_set_drvdata(spi, &myspi);	/* 将myspi添加到spi从机设备对象的私有数据区中 */

	return 0;

out3:
	class_destroy(myspi.class);

out2:
	cdev_del(&myspi.cdev);

out1:
	unregister_chrdev_region(myspi.devid, 1);

	return ret;
}

static int myspi_remove(struct spi_device *spi)
{
	struct myspi_dev *dev = spi_get_drvdata(spi);

	/* 注销设备 */
	device_destroy(dev->class, dev->devid);

	/* 注销类 */
	class_destroy(dev->class);

	/* 删除cdev */
	cdev_del(&dev->cdev);

	/* 注销设备号 */
	unregister_chrdev_region(dev->devid, 1);

	return 0;
}

/* 匹配列表 */
static const struct of_device_id myspi_of_match[] = {
	{ .compatible = "alientek,spidev" },
	{ /* Sentinel */ }
};

/* SPI总线下的设备驱动结构体变量 */
static struct spi_driver myspi_driver = {
	.driver = {
		.name			= "myspi",
		.of_match_table	= myspi_of_match,
	},
	.probe		= myspi_probe,		// probe函数
	.remove		= myspi_remove,		// remove函数
};

module_spi_driver(myspi_driver);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Virtual Device Driver Based On SPI Subsystem");
MODULE_LICENSE("GPL");
