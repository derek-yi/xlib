/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : keyinput.c
 作者      : 邓涛
 版本      : V1.0
 描述      : input子系统驱动框架编程示例之按键驱动
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#define PS_KEY0_CODE		KEY_0

/* 自定义的按键设备结构体 */
struct mykey_dev {
	struct input_dev *idev;		// 按键对应的input_dev指针
	struct timer_list timer;	// 消抖定时器
	int gpio;					// 按键对应的gpio编号
	int irq;					// 按键对应的中断号
};

/*
 * @description			: 定时器服务函数，用于按键消抖，定时时间到了以后
 * 						  再读取按键值，根据按键的状态上报相应的事件
 * @param - arg			: arg参数可以在初始化定时器的时候进行配置
 * @return				: 无
 */
static void key_timer_function(unsigned long arg)
{
	struct mykey_dev *key = (struct mykey_dev *)arg;
	int val;

	/* 读取按键值并上报按键事件 */
	val = gpio_get_value(key->gpio);
	input_report_key(key->idev, PS_KEY0_CODE, !val);// 上报按键事件
	input_sync(key->idev);							// 同步事件

	/* 使能按键中断 */
	enable_irq(key->irq);
}

/*
 * @description			: 按键中断服务函数
 * @param - irq			: 触发该中断事件对应的中断号
 * @param - arg			: arg参数可以在申请中断的时候进行配置
 * @return				: 中断执行结果
 */

static irqreturn_t mykey_interrupt(int irq, void *arg)
{
	struct mykey_dev *key = (struct mykey_dev *)arg;

	/* 判断触发中断的中断号是否是按键对应的中断号 */
	if (key->irq != irq)
		return IRQ_NONE;

	/* 按键防抖处理，开启定时器延时15ms */
	disable_irq_nosync(irq);		// 禁止按键中断
	mod_timer(&key->timer, jiffies + msecs_to_jiffies(15));

	return IRQ_HANDLED;
}

/*
 * @description			: 按键初始化函数
 * @param - pdev		: platform设备指针
 * @return				: 成功返回0，失败返回负数
 */
static int mykey_init(struct platform_device *pdev)
{
	struct mykey_dev *key = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	unsigned long irq_flags;
	int ret;

	/* 从设备树中获取GPIO */
	key->gpio = of_get_named_gpio(dev->of_node, "key-gpio", 0);
	if(!gpio_is_valid(key->gpio)) {
		dev_err(dev, "Failed to get gpio");
		return -EINVAL;
	}

	/* 申请使用GPIO */
	ret = devm_gpio_request(dev, key->gpio, "Key Gpio");
	if (ret) {
		dev_err(dev, "Failed to request gpio");
		return ret;
	}

	/* 将GPIO设置为输入模式 */
	gpio_direction_input(key->gpio);

	/* 获取GPIO对应的中断号 */
	key->irq = irq_of_parse_and_map(dev->of_node, 0);
	if (!key->irq)
		return -EINVAL;

	/* 获取设备树中指定的中断触发类型 */
	irq_flags = irq_get_trigger_type(key->irq);
	if (IRQF_TRIGGER_NONE == irq_flags)
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;

	/* 申请中断 */
	return devm_request_irq(dev, key->irq, mykey_interrupt,
				irq_flags, "PS_Key0 IRQ", key);
}

/*
 * @description			: platform驱动的probe函数，当驱动与设备
 * 						  匹配成功以后此函数会被执行
 * @param - pdev		: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int mykey_probe(struct platform_device *pdev)
{
	struct mykey_dev *key;
	struct input_dev *idev;
	int ret;

	dev_info(&pdev->dev, "Key device and driver matched successfully!\n");

	/* 为key指针分配内存 */
	key = devm_kzalloc(&pdev->dev, sizeof(struct mykey_dev), GFP_KERNEL);
	if (!key)
		return -ENOMEM;

	platform_set_drvdata(pdev, key);

	/* 初始化按键 */
	ret = mykey_init(pdev);
	if (ret)
		return ret;

	/* 定时器初始化 */
	init_timer(&key->timer);
	key->timer.function	= key_timer_function;
	key->timer.data		= (unsigned long)key;

	/* input_dev初始化 */
	idev = devm_input_allocate_device(&pdev->dev);
	if (!idev)
		return -ENOMEM;

	key->idev = idev;
	idev->name = "mykey";

	__set_bit(EV_KEY, idev->evbit);			// 可产生按键事件
	__set_bit(EV_REP, idev->evbit);			// 可产生重复事件
	__set_bit(PS_KEY0_CODE, idev->keybit);	// 可产生KEY_0按键事件

	/* 注册按键输入设备 */
	return input_register_device(idev);
}

/*
 * @description			: platform驱动的remove函数，当platform驱动模块
 * 						  卸载时此函数会被执行
 * @param - dev			: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int mykey_remove(struct platform_device *pdev)
{
	struct mykey_dev *key = platform_get_drvdata(pdev);

	/* 删除定时器 */
	del_timer_sync(&key->timer);

	/* 卸载按键设备 */
	input_unregister_device(key->idev);

	return 0;
}

/* 匹配列表 */
static const struct of_device_id key_of_match[] = {
	{ .compatible = "alientek,key" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver mykey_driver = {
	.driver = {
		.name			= "zynq-key",		// 驱动名字，用于和设备匹配
		.of_match_table	= key_of_match,		// 设备树匹配表，用于和设备树中定义的设备匹配
	},
	.probe		= mykey_probe,		// probe函数
	.remove		= mykey_remove,		// remove函数
};

module_platform_driver(mykey_driver);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("Key Driver Based On Input Subsystem");
MODULE_LICENSE("GPL");
