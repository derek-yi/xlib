/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : pcf8563.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 基于Linux I2C总线框架的pcf8563驱动程序
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/bcd.h>
#include <linux/uaccess.h>

#define DEVICE_NAME		"pcf8563"	/* 名字 */

/*
 * pcf8563内部寄存器定义
 */
#define PCF8563_CTL_STATUS_1	0x00	/* 控制寄存器1 */
#define PCF8563_CTL_STATUS_2	0x01	/* 控制寄存器2 */
#define PCF8563_VL_SECONDS		0x02	/* 时间: 秒 */
#define PCF8563_MINUTES			0x03	/* 时间: 分 */
#define PCF8563_HOURS			0x04	/* 时间: 小时 */
#define PCF8563_DAYS			0x05	/* 日期: 天 */
#define PCF8563_WEEKDAYS		0x06	/* 日期: 星期 */
#define PCF8563_CENTURY_MONTHS	0x07	/* 日期: 月 */
#define PCF8563_YEARS			0x08	/* 日期: 年 */

#define YEAR_BASE				2000	/* 20xx年 */

/*
 * 自定义结构体，用于表示时间和日期信息
 */
struct pcf8563_time {
	int sec;	// 秒
	int min;	// 分
	int hour;	// 小时
	int day;	// 日
	int wday;	// 星期
	int mon;	// 月份
	int year;	// 年
};

/*
 * 自定义结构体pcf8563_dev
 * 用于描述pcf8563设备
 */
struct pcf8563_dev {
	struct i2c_client *client;		/* i2c次设备 */
	dev_t devid;					/* 设备号 */
	struct cdev cdev;				/* cdev结构体 */
	struct class *class;			/* 类 */
	struct device *device;			/* 设备 */
};

static struct pcf8563_dev pcf8563;	// 定义一个pcf8563设备

/*
 * @description			: 向pcf8563设备多个连续的寄存器写入数据
 * @param – dev			: pcf8563设备
 * @param – reg			: 要写入的寄存器首地址
 * @param – buf			: 待写入的数据缓存区地址
 * @param – len			: 需要写入的字节长度
 * @return				: 成功返回0，失败返回一个负数
 */
static int pcf8563_write_reg(struct pcf8563_dev *dev, u8 reg, u8 *buf, u8 len)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg;
	u8 send_buf[17] = {0};
	int ret;

	if (16 < len) {
		dev_err(&client->dev, "%s: error: Invalid transfer byte length %d\n",
					__func__, len);
		return -EINVAL;
	}

	send_buf[0] = reg;				// 寄存器首地址
	memcpy(&send_buf[1], buf, len);	// 将要写入的数据存放到数组send_buf后面

	msg.addr = client->addr;		// pcf8563从机地址
	msg.flags = client->flags;		// 标记为写数据
	msg.buf = send_buf;				// 要写入的数据缓冲区
	msg.len = len + 1;				// 要写入的数据长度

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (1 != ret) {
		dev_err(&client->dev, "%s: error: reg=0x%x, len=0x%x\n",
					__func__, reg, len);
		return -EIO;
	}

	return 0;
}

/*
 * @description			: 从pcf8563设备中读取多个连续的寄存器数据
 * @param – dev			: pcf8563设备
 * @param – reg			: 要读取的寄存器首地址
 * @param – buf			: 数据存放缓存区地址
 * @param – len			: 读取的字节长度
 * @return				: 成功返回0，失败返回一个负数
 */
static int pcf8563_read_reg(struct pcf8563_dev *dev, u8 reg, u8 *buf, u8 len)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg[2];
	int ret;

	/* msg[0]: 发送消息 */
	msg[0].addr = client->addr;		// pcf8563从机地址
	msg[0].flags = client->flags;	// 标记为写数据
	msg[0].buf = &reg;				// 要写入的数据缓冲区
	msg[0].len = 1;					// 要写入的数据长度

	/* msg[1]: 接收消息 */
	msg[1].addr = client->addr;		// pcf8563从机地址
	msg[1].flags = client->flags | I2C_M_RD;// 标记为读数据
	msg[1].buf = buf;				// 存放读数据的缓冲区
	msg[1].len = len;				// 读取的字节长度

	ret = i2c_transfer(client->adapter, msg, 2);
	if (2 != ret) {
		dev_err(&client->dev, "%s: error: reg=0x%x, len=0x%x\n",
					__func__, reg, len);
		return -EIO;
	}

	return 0;
}

/*
 * @description			: 打开设备
 * @param – inode		: 传递给驱动的inode
 * @param - filp		: 设备文件，file结构体有个叫做private_data的成员变量
 * 						  一般在open的时候将private_data指向设备结构体。
 * @return				: 0 成功;其他 失败
 */
static int pcf8563_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &pcf8563;
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
static ssize_t pcf8563_read(struct file *filp, char __user *buf,
			size_t cnt, loff_t *off)
{
	struct pcf8563_dev *dev = filp->private_data;
	struct i2c_client *client = dev->client;
	struct pcf8563_time time = {0};
	u8 read_buf[9] = {0};
	int ret;

	/* 读寄存器数据 */
	ret = pcf8563_read_reg(dev, PCF8563_CTL_STATUS_1,
				read_buf, 9);
	if (ret)
		return ret;

	/* 校验时钟完整性 */
	if (read_buf[PCF8563_VL_SECONDS] & 0x80) {
		dev_err(&client->dev,
					"low voltage detected, date/time is not reliable.\n");
		return -EINVAL;
	}

	/* 将BCD码转换为数据得到时间、日期 */
	time.sec = bcd2bin(read_buf[PCF8563_VL_SECONDS] & 0x7F);		// 秒
	time.min = bcd2bin(read_buf[PCF8563_MINUTES] & 0x7F);			// 分
	time.hour = bcd2bin(read_buf[PCF8563_HOURS] & 0x3F);			// 小时
	time.day = bcd2bin(read_buf[PCF8563_DAYS] & 0x3F);				// 日
	time.wday = read_buf[PCF8563_WEEKDAYS] & 0x07;					// 星期
	time.mon = bcd2bin(read_buf[PCF8563_CENTURY_MONTHS] & 0x1F);	// 月
	time.year = bcd2bin(read_buf[PCF8563_YEARS]) + YEAR_BASE;		// 年

	/* 将数据拷贝到用户空间 */
	return copy_to_user(buf, &time, sizeof(struct pcf8563_time));
}

/*
 * @description			: 向设备写数据 
 * @param – filp		: 设备文件，表示打开的文件描述符
 * @param - buf			: 要写给设备写入的数据
 * @param - cnt			: 要写入的数据长度
 * @param - offt		: 相对于文件首地址的偏移
 * @return				: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t pcf8563_write(struct file *filp, const char __user *buf,
			size_t cnt, loff_t *offt)
{
	struct pcf8563_dev *dev = filp->private_data;
	struct pcf8563_time time = {0};
	u8 write_buf[9] = {0};
	int ret;

	ret = copy_from_user(&time, buf, cnt);	// 得到应用层传递过来的数据
	if(0 > ret)
		return -EFAULT;

	/* 将数据转换为BCD码 */
	write_buf[PCF8563_VL_SECONDS] = bin2bcd(time.sec);		// 秒
	write_buf[PCF8563_MINUTES] = bin2bcd(time.min);			// 分
	write_buf[PCF8563_HOURS] = bin2bcd(time.hour);			// 小时
	write_buf[PCF8563_DAYS] = bin2bcd(time.day);			// 日
	write_buf[PCF8563_WEEKDAYS] = time.wday & 0x07;			// 星期
	write_buf[PCF8563_CENTURY_MONTHS] = bin2bcd(time.mon);	// 月
	write_buf[PCF8563_YEARS] = bin2bcd(time.year % 100);	// 年

	/* 将数据写入寄存器 */
	ret = pcf8563_write_reg(dev, PCF8563_VL_SECONDS,
				&write_buf[PCF8563_VL_SECONDS], 7);
	if (ret)
		return ret;

	return cnt;
}

/*
 * @description			: 关闭/释放设备
 * @param - filp		: 要关闭的设备文件(文件描述符)
 * @return				: 0 成功;其他 失败
 */
static int pcf8563_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * file_operations结构体变量
 */
static const struct file_operations pcf8563_ops = {
	.owner = THIS_MODULE,
	.open = pcf8563_open,
	.read = pcf8563_read,
	.write = pcf8563_write,
	.release = pcf8563_release,
};

static int pcf8563_init(struct pcf8563_dev *dev)
{
	u8 val;
	int ret;

	ret = pcf8563_read_reg(dev, PCF8563_VL_SECONDS, &val, 1);
	if (ret)
		return ret;

	val &= 0x7F;

	return pcf8563_write_reg(dev, PCF8563_VL_SECONDS, &val, 1);
}

static int pcf8563_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;

	/* 初始化pcf8563 */
	pcf8563.client = client;
	ret = pcf8563_init(&pcf8563);
	if (ret)
		return ret;

	/* 申请设备号 */
	ret = alloc_chrdev_region(&pcf8563.devid, 0, 1, DEVICE_NAME);
	if (ret)
		return ret;

	/* 初始化字符设备cdev */
	pcf8563.cdev.owner = THIS_MODULE;
	cdev_init(&pcf8563.cdev, &pcf8563_ops);

	/* 添加cdev */
	ret = cdev_add(&pcf8563.cdev, pcf8563.devid, 1);
	if (ret)
		goto out1;

	/* 创建类class */
	pcf8563.class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(pcf8563.class)) {
		ret = PTR_ERR(pcf8563.class);
		goto out2;
	}

	/* 创建设备 */
	pcf8563.device = device_create(pcf8563.class, &client->dev,
				pcf8563.devid, NULL, DEVICE_NAME);
	if (IS_ERR(pcf8563.device)) {
		ret = PTR_ERR(pcf8563.device);
		goto out3;
	}

	i2c_set_clientdata(client, &pcf8563);

	return 0;

out3:
	class_destroy(pcf8563.class);

out2:
	cdev_del(&pcf8563.cdev);

out1:
	unregister_chrdev_region(pcf8563.devid, 1);

	return ret;
}

static int pcf8563_remove(struct i2c_client *client)
{
	struct pcf8563_dev *pcf8563 = i2c_get_clientdata(client);

	/* 注销设备 */
	device_destroy(pcf8563->class, pcf8563->devid);

	/* 注销类 */
	class_destroy(pcf8563->class);

	/* 删除cdev */
	cdev_del(&pcf8563->cdev);

	/* 注销设备号 */
	unregister_chrdev_region(pcf8563->devid, 1);

	return 0;
}

/* 匹配列表 */
static const struct of_device_id pcf8563_of_match[] = {
	{ .compatible = "zynq-pcf8563" },
	{ /* Sentinel */ }
};

/* i2c_driver结构体变量 */
static struct i2c_driver pcf8563_driver = {
	.driver = {
		.name			= "pcf8563",
		.of_match_table	= pcf8563_of_match,
	},
	.probe		= pcf8563_probe,		// probe函数
	.remove		= pcf8563_remove,		// remove函数
};

module_i2c_driver(pcf8563_driver);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Philips PCF8563 RTC driver");
MODULE_LICENSE("GPL");
