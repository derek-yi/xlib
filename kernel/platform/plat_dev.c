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
 
static struct platform_device plat_led_dev = {
    .name = "plat_led_dev",
    .id = -1,
    .resource = plat_led_resource,
    .num_resources = ARRAY_SIZE(plat_led_resource),
    .dev = {
        .release = plat_led_dev_release,
    }
};

static int led_dev_init(void)
{
    platform_device_register(&plat_led_dev);
    return 0;
}

static void led_dev_exit(void)
{
    platform_device_unregister(&plat_led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek");


