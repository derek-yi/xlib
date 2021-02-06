/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : newchrled.c
 作者      : 邓涛
 版本      : V1.0
 描述      : ZYNQ LED驱动文件。
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

#define NEWCHRLED_CNT		1				/* 设备号个数 */
#define NEWCHRLED_NAME		"newchrled"		/* 名字 */

/* 
 * GPIO相关寄存器地址定义
 */
#define ZYNQ_GPIO_REG_BASE			0xE000A000
#define DATA_OFFSET					0x00000040
#define DIRM_OFFSET					0x00000204
#define OUTEN_OFFSET				0x00000208
#define INTDIS_OFFSET				0x00000214
#define APER_CLK_CTRL				0xF800012C

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *data_addr;
static void __iomem *dirm_addr;
static void __iomem *outen_addr;
static void __iomem *intdis_addr;
static void __iomem *aper_clk_ctrl_addr;

/* newchrled设备结构体 */
struct newchrled_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
	int major;				/* 主设备号 */
	int minor;				/* 次设备号 */
};

static struct newchrled_dev newchrled;	/* led设备 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &newchrled;	/* 设置私有数据 */
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
	int val;
	char kern_buf[1];

	ret = copy_from_user(kern_buf, buf, cnt);	// 得到应用层传递过来的数据
	if(0 > ret) {
		printk(KERN_ERR "kernel write failed!\r\n");
		return -EFAULT;
	}

	val = readl(data_addr);
	if (0 == kern_buf[0])
		val &= ~(0x1U << 7);		// 如果传递过来的数据是0则关闭led
	else if (1 == kern_buf[0])
		val |= (0x1U << 7);			// 如果传递过来的数据是1则点亮led

	writel(val, data_addr);
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

static inline void led_ioremap(void)
{
	data_addr = ioremap(ZYNQ_GPIO_REG_BASE + DATA_OFFSET, 4);
	dirm_addr = ioremap(ZYNQ_GPIO_REG_BASE + DIRM_OFFSET, 4);
	outen_addr = ioremap(ZYNQ_GPIO_REG_BASE + OUTEN_OFFSET, 4);
	intdis_addr = ioremap(ZYNQ_GPIO_REG_BASE + INTDIS_OFFSET, 4);
	aper_clk_ctrl_addr = ioremap(APER_CLK_CTRL, 4);
}

static inline void led_iounmap(void)
{
	iounmap(data_addr);
	iounmap(dirm_addr);
	iounmap(outen_addr);
	iounmap(intdis_addr);
	iounmap(aper_clk_ctrl_addr);
}

/* 设备操作函数 */
static struct file_operations newchrled_fops = {
	.owner		= THIS_MODULE,
	.open		= led_open,
	.read		= led_read,
	.write		= led_write,
	.release	= led_release,
};

static int __init led_init(void)
{
	u32 val;
	int ret;

	/* 1.寄存器地址映射 */
	led_ioremap();

	/* 2.使能GPIO时钟 */
	val = readl(aper_clk_ctrl_addr);
	val |= (0x1U << 22);
	writel(val, aper_clk_ctrl_addr);

	/* 3.关闭中断功能 */
	val |= (0x1U << 7);
	writel(val, intdis_addr);

	/* 4.设置GPIO为输出功能 */
	val = readl(dirm_addr);
	val |= (0x1U << 7);
	writel(val, dirm_addr);

	/* 5.使能GPIO输出功能 */
	val = readl(outen_addr);
	val |= (0x1U << 7);
	writel(val, outen_addr);

	/* 6.默认关闭LED */
	val = readl(data_addr);
	val &= ~(0x1U << 7);
	writel(val, data_addr);

	/* 7.注册字符设备驱动 */
	 /* 创建设备号 */
	if (newchrled.major) {
		newchrled.devid = MKDEV(newchrled.major, 0);
		ret = register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
		if (ret)
			goto out1;
	} else {
		ret = alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
		if (ret)
			goto out1;

		newchrled.major = MAJOR(newchrled.devid);
		newchrled.minor = MINOR(newchrled.devid);
	}

	printk("newcheled major=%d,minor=%d\r\n",newchrled.major, newchrled.minor); 

	 /* 初始化cdev */
	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &newchrled_fops);

	 /* 添加一个cdev */
	ret = cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);
	if (ret)
		goto out2;

	 /* 创建类 */
	newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.class)) {
		ret = PTR_ERR(newchrled.class);
		goto out3;
	}

	 /* 创建设备 */
	newchrled.device = device_create(newchrled.class, NULL,
				newchrled.devid, NULL, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.device)) {
		ret = PTR_ERR(newchrled.device);
		goto out4;
	}

	return 0;

out4:
	class_destroy(newchrled.class);

out3:
	cdev_del(&newchrled.cdev);

out2:
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);

out1:
	led_iounmap();

	return ret;
}

static void __exit led_exit(void)
{
	/* 注销设备 */
	device_destroy(newchrled.class, newchrled.devid);

	/* 注销类 */
	class_destroy(newchrled.class);

	/* 删除cdev */
	cdev_del(&newchrled.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);

	/* 取消地址映射 */
	led_iounmap();
}

/* 驱动模块入口和出口函数注册 */
module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Alientek ZYNQ GPIO LED Driver");
MODULE_LICENSE("GPL");
