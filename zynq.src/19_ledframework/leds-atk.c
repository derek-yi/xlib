/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : leds-atk.c
 作者      : 邓涛
 版本      : V1.0
 描述      : LED驱动框架编程示例
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2019/1/30 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

/* LED数据资源结构体 */
struct myled_data {
	struct led_classdev cdev;	// led设备
	int gpio;					// gpio编号
};

/*
 * @description			: 静态内敛函数，该函数通过struct myled_data结构体中cdev变量的地址
 * 						: 得到struct myled_data结构体变量的地址
 * @param - led_cdev	: struct myled_data结构体中cdev变量的地址
 * @return				: 执行成功返回struct myled_data结构体变量的地址
 */
static inline struct myled_data *
			cdev_to_led_data(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct myled_data, cdev);
}

/*
 * @description			: LED相关初始化操作
 * @param - pdev		: struct platform_device指针，也就是platform设备指针
 * @return				: 成功返回0，失败返回负数
 */
static int myled_init(struct platform_device *pdev)
{
	struct myled_data *led_data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int ret;

	/* 从设备树中获取GPIO */
	led_data->gpio = of_get_named_gpio(dev->of_node, "led-gpio", 0);
	if(!gpio_is_valid(led_data->gpio)) {
		dev_err(dev, "Failed to get gpio");
		return -EINVAL;
	}

	/* 申请使用GPIO */
	ret = devm_gpio_request(dev, led_data->gpio, "PS_LED0 Gpio");
	if (ret) {
		dev_err(dev, "Failed to request gpio");
		return ret;
	}

	/* 将GPIO设置为输出模式并将输出低电平 */
	gpio_direction_output(led_data->gpio, 0);

	return 0;
}

/*
 * @description			: 用于设置LED的亮度，不可休眠
 * @param - led_cdev	: struct led_classdev类型指针
 * @param - value		: 亮度值
 * @return				: 无
 */
static void myled_brightness_set(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	struct myled_data *led_data = cdev_to_led_data(led_cdev);
	int level;

	if (value == LED_OFF)
		level = 0;
	else
		level = 1;

	gpio_set_value(led_data->gpio, level);
}

/*
 * @description			: 用于设置LED的亮度，可以休眠
 * @param - led_cdev	: struct led_classdev类型指针
 * @param - value		: 亮度值
 * @return				: 无
 */
static int myled_brightness_set_blocking(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	myled_brightness_set(led_cdev, value);
	return 0;
}

/*
 * @description			: platform驱动的probe函数，当驱动与设备
 * 						  匹配成功以后此函数就会执行
 * @param - pdev		: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int myled_probe(struct platform_device *pdev)
{
	struct myled_data *led_data;
	struct led_classdev *led_cdev;
	int ret;

	dev_info(&pdev->dev, "LED device and driver matched successfully!\n");

	/* 为led_data指针分配内存 */
	led_data = devm_kzalloc(&pdev->dev, sizeof(struct myled_data), GFP_KERNEL);
	if (!led_data)
		return -ENOMEM;

	platform_set_drvdata(pdev, led_data);

	/* 初始化LED */
	ret = myled_init(pdev);
	if (ret)
		return ret;

	/* 初始化led_cdev变量 */
	led_cdev = &led_data->cdev;
	led_cdev->name = "myled";				// 设置设备名字
	led_cdev->brightness = LED_OFF;			// 设置LED初始亮度
	led_cdev->max_brightness = LED_FULL;	// 设置LED最大亮度
	led_cdev->brightness_set = myled_brightness_set;	// 绑定LED亮度设置函数（不可休眠）
	led_cdev->brightness_set_blocking = myled_brightness_set_blocking;	// 绑定LED亮度设置函数（可以休眠）

	/* 注册LED设备 */
	return led_classdev_register(&pdev->dev, led_cdev);
}

/*
 * @description			: platform驱动模块卸载时此函数会执行
 * @param - dev			: platform设备指针
 * @return				: 0，成功;其他负值,失败
 */
static int myled_remove(struct platform_device *pdev)
{
	struct myled_data *led_data = platform_get_drvdata(pdev);
	led_classdev_unregister(&led_data->cdev);
	dev_info(&pdev->dev, "LED driver has been removed!\n");
	return 0;
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
	{ .compatible = "alientek,led" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver myled_driver = {
	.driver = {
		.name			= "zynq-led",		// 驱动名字，用于和设备匹配
		.of_match_table	= led_of_match,		// 设备树匹配表，用于和设备树中定义的设备匹配
	},
	.probe		= myled_probe,	// probe函数
	.remove		= myled_remove,	// remove函数
};

module_platform_driver(myled_driver);

MODULE_AUTHOR("DengTao <773904075@qq.com>");
MODULE_DESCRIPTION("LED Driver Based on LED Driver Framework");
MODULE_LICENSE("GPL");
