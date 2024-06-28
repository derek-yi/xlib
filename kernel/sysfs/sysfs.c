#include <linux/init.h>             
#include <linux/module.h>           
#include <linux/kernel.h>
#include <linux/kthread.h>      
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>


static int led_status = 0;

#define LED_PIN   26

static struct kobject *my_kobj;

#if 1
static ssize_t led_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    printk(KERN_INFO "led_show\n");
    return sprintf(buf, "The led status = %d\n", led_status);
}
#endif

static ssize_t led_status_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    printk(KERN_INFO "led_status_show\n");
    return sprintf(buf, "led status: %d\n", led_status);
}

static ssize_t led_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    printk(KERN_INFO "led_status_store\n");
    
    if(0 == memcmp(buf, "on", 2))
    {
        //gpio_set_value(LED_PIN,1);
        led_status = 1;
    }
    else if(0 == memcmp(buf, "off", 3))
    {
        //gpio_set_value(LED_PIN, 0);
        led_status = 0;
    }
    else
    {
        printk(KERN_INFO "Not support cmd\n");
    }
    
    return count;
}

static struct kobj_attribute status_attr = __ATTR_RO(led);
static struct kobj_attribute led_attr = __ATTR(led_status, 0660, led_status_show, led_status_store);  

static struct attribute *led_attrs[] = {
    &status_attr.attr,
    &led_attr.attr,
    NULL,
};

static struct attribute_group led_attr_grp = {
    .name = "led_test",
    .attrs = led_attrs,
};

static int __init sysfs_ctrl_init(void)
{
    printk(KERN_INFO "sysfs_ctrl_init\n");
    
    my_kobj = kobject_create_and_add("obj_test", kernel_kobj->parent);
    
    sysfs_create_group(my_kobj, &led_attr_grp);
    
    return 0;
}

static void __exit sysfs_ctrl_exit(void)
{
    sysfs_remove_group(my_kobj, &led_attr_grp);
    
    kobject_put(my_kobj);
    
    printk(KERN_INFO "sysfs_ctrl_exit\n");
}

/*
cat /sys/obj_test/led_test/led
cat /sys/obj_test/led_test/led_status
chmod 666 /sys/obj_test/led_test/led_status
echo "off" > /sys/obj_test/led_test/led_status
echo "on" > /sys/obj_test/led_test/led_status
*/

module_init(sysfs_ctrl_init);
module_exit(sysfs_ctrl_exit);

MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  

