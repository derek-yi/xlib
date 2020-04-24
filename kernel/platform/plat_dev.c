/* 本文件是依照platform驱动<详细步骤>章节编写，本文件
 * 的目的是编写和介绍具体代码，不介绍框架
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/leds.h>
#include <linux/rfkill.h>
#include <linux/pci.h>
#include <linux/pci_hotplug.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>


/* 2.2.3定义设备资源数组，类型为struct resource，放入内存资源和中断资源 */
static struct resource plat_led_resource[] = {
    [0] = {
          .start = 0x56000050,
          .end   = 0x56000050 + 8 - 1,
          .flags = IORESOURCE_MEM,
    },
    [1] = {
          .start = 3,
          .end   = 3,
          .flags = IORESOURCE_IRQ,
    },    
};

static void plat_led_dev_release(struct device *dev)
{
    printk("plat_led_dev_release\n");
}
 
/* 2.2.1定义并led设备，类型为struct platform_device */
/* 2.2.2填充led设备，放入设备名字，设备id，设备资源，资源个数，release函数 */
static struct platform_device plat_led_dev = {
    .name = "plat_led_dev",
    .id = -1,
    .resource = plat_led_resource,
    .num_resources = ARRAY_SIZE(plat_led_resource),
    .dev = {
        .release = plat_led_dev_release,
    }
};

/* 2.2在入口函数中注册led设备，类型为struct platform_device */
static int led_dev_init(void)
{
    platform_device_register(&plat_led_dev);
    return 0;
}

/* 2.3在出口函数中卸载platform_device类型的led设备 */
static void led_dev_exit(void)
{
    platform_device_unregister(&plat_led_dev);
}

/* 2.1编写代码框架：头文件，入口函数，出口函数，声明LICENSE为GPL */
module_init(led_dev_init);
module_exit(led_dev_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek");


