/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : leddriver.c
 作者      : 邓涛
 版本      : V1.0
 描述      : platform总线编程示例之platform驱动模块
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#define MYLED_CNT		1			/* 设备号个数 */
#define MYLED_NAME		"myled"		/* 名字 */

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *data_addr;
static void __iomem *dirm_addr;
static void __iomem *outen_addr;
static void __iomem *intdis_addr;
static void __iomem *aper_clk_ctrl_addr;

/* LED设备结构体 */
struct myled_dev {
	dev_t devid;			/* 设备号 */
	struct cdev cdev;		/* cdev结构体 */
	struct class *class;	/* 类 */
	struct device *device;	/* 设备 */
};

static struct myled_dev myled;		/* led设备 */

/*
 * @description		: 打开设备
 * @param – inode	: 传递给驱动的inode
 * @param - filp	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return			: 0 成功;其他 失败
 */
static int myled_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param – filp	: 设备文件，表示打开的文件描述符
 * @param - buf		: 要写给设备写入的数据
 * @param - cnt		: 要写入的数据长度
 * @param - offt	: 相对于文件首地址的偏移
 * @return			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t myled_write(struct file *filp, const char __user *buf, 
			size_t cnt, loff_t *offt)
{
	int ret;
	int val;
	char kern_buf[1];

	ret = copy_from_user(kern_buf, buf, cnt);		// 得到应用层传递过来的数据
	if(0 > ret) {
		printk(KERN_ERR "myled: kernel write failed!\r\n");
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

static int myled_get_platform_resource(struct platform_device *dev)
{
	int i;
	struct resource *res[5];

	/* 获取资源 */
	for (i = 0; i < 5; i++) {

		res[i] = platform_get_resource(dev, IORESOURCE_MEM, i); 
		if (!res[i]) {
			printk(KERN_ERR "no MEM resource found %d\n", i);
			return -ENXIO;
		}
	}

	/* 将物理地址映射为虚拟地址 */
	data_addr = ioremap(res[0]->start, resource_size(res[0]));
	dirm_addr = ioremap(res[1]->start, resource_size(res[1]));
	outen_addr = ioremap(res[2]->start, resource_size(res[2]));
	intdis_addr = ioremap(res[3]->start, resource_size(res[3]));
	aper_clk_ctrl_addr = ioremap(res[4]->start, resource_size(res[4]));

	return 0;
}

static void myled_init(void)
{
	u32 val;

	/* 使能GPIO时钟 */
	val = readl(aper_clk_ctrl_addr);
	val |= (0x1U << 22);
	writel(val, aper_clk_ctrl_addr);

	/* 关闭中断功能 */
	val |= (0x1U << 7);
	writel(val, intdis_addr);

	/* 设置GPIO为输出功能 */
	val = readl(dirm_addr);
	val |= (0x1U << 7);
	writel(val, dirm_addr);

	/* 使能GPIO输出功能 */
	val = readl(outen_addr);
	val |= (0x1U << 7);
	writel(val, outen_addr);

	/* 默认关闭LED */
	val = readl(data_addr);
	val &= ~(0x1U << 7);
	writel(val, data_addr);
}

static void myled_iounmap(void)
{
	iounmap(data_addr);
	iounmap(dirm_addr);
	iounmap(outen_addr);
	iounmap(intdis_addr);
	iounmap(aper_clk_ctrl_addr);
}

/* LED设备操作函数 */
static struct file_operations myled_fops = {
	.owner = THIS_MODULE,
	.open = myled_open,
	.write = myled_write,
};

/*
 * @description		: platform驱动的probe函数，当platform驱动与platform设备
 * 					  匹配以后此函数就会执行
 * @param - dev		: platform设备指针
 * @return			: 0，成功;其他负值,失败
 */
static int myled_probe(struct platform_device *dev)
{
	int ret;

	printk(KERN_INFO "myled: led driver and device has matched!\r\n");

	/* 获取platform设备资源 */
	ret = myled_get_platform_resource(dev);
	if (ret)
		return ret;

	/* led初始化 */
	myled_init();

	/* 初始化cdev */
	ret = alloc_chrdev_region(&myled.devid, 0, MYLED_CNT, MYLED_NAME);
	if (ret)
		goto out1;

	myled.cdev.owner = THIS_MODULE;
	cdev_init(&myled.cdev, &myled_fops);

	/* 添加cdev */
	ret = cdev_add(&myled.cdev, myled.devid, MYLED_CNT);
	if (ret)
		goto out2;

	/* 创建类class */
	myled.class = class_create(THIS_MODULE, MYLED_NAME);
	if (IS_ERR(myled.class)) {
		ret = PTR_ERR(myled.class);
		goto out3;
	}

	/* 创建设备 */
	myled.device = device_create(myled.class, &dev->dev,
				myled.devid, NULL, MYLED_NAME);
	if (IS_ERR(myled.device)) {
		ret = PTR_ERR(myled.device);
		goto out4;
	}

	return 0;

out4:
	class_destroy(myled.class);

out3:
	cdev_del(&myled.cdev);

out2:
	unregister_chrdev_region(myled.devid, MYLED_CNT);

out1:
	myled_iounmap();

	return ret;
}

/*
 * @description		: platform驱动卸载时此函数会执行
 * @param - dev		: platform设备指针
 * @return			: 0，成功;其他负值,失败
 */
static int myled_remove(struct platform_device *dev)
{
	printk(KERN_INFO "myled: led platform driver remove!\r\n");

	/* 注销设备 */
	device_destroy(myled.class, myled.devid);

	/* 注销类 */
	class_destroy(myled.class);

	/* 删除cdev */
	cdev_del(&myled.cdev);

	/* 注销设备号 */
	unregister_chrdev_region(myled.devid, MYLED_CNT);

	/* 删除地址映射 */
	myled_iounmap();

	return 0;
}

/* platform驱动结构体 */
static struct platform_driver myled_driver = {
	.driver = {
		.name	= "zynq-led",	// 驱动名字，用于和设备匹配
	},
	.probe		= myled_probe,	// probe函数
	.remove		= myled_remove,	// remove函数
};

/*
 * @description		: 模块入口函数
 * @param			: 无
 * @return			: 无
 */
static int __init myled_driver_init(void)
{
	return platform_driver_register(&myled_driver);
}

/*
 * @description		: 模块出口函数
 * @param			: 无
 * @return			: 无
 */
static void __exit myled_driver_exit(void)
{
	platform_driver_unregister(&myled_driver);
}

module_init(myled_driver_init);
module_exit(myled_driver_exit);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Led Platform Driver");
MODULE_LICENSE("GPL");
