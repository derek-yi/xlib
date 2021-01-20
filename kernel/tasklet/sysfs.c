
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


static int nix_status = 0;

static struct kobject *my_kobj;

static ssize_t nix_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    printk(KERN_INFO "nix_show\n");
    return sprintf(buf, "nix_status = %d\n", nix_status);
}

static ssize_t nix_status_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    printk(KERN_INFO "nix_status_show\n");
    return sprintf(buf, "nix_status: %d\n", nix_status);
}

static ssize_t nix_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    printk(KERN_INFO "nix_status_store\n");
    
    if (0 == memcmp(buf, "on", 2)) 
    {
        nix_status = 1;
    } 
    else if (0 == memcmp(buf, "off", 3)) 
    {
        nix_status = 0;
    }
    
    return count;
}

static struct kobj_attribute status_attr = __ATTR_RO(nix);
static struct kobj_attribute nix_attr = __ATTR(nix_status, 0660, nix_status_show, nix_status_store);  

static struct attribute *nix_attrs[] = {
    &status_attr.attr,
    &nix_attr.attr,
    NULL,
};

static struct attribute_group nix_attr_grp = {
    .name = "nix_test",
    .attrs = nix_attrs,
};

static int __init sysfs_ctrl_init(void)
{
    printk(KERN_INFO "sysfs_ctrl_init\n");
    my_kobj = kobject_create_and_add("derek", kernel_kobj->parent);
    sysfs_create_group(my_kobj, &nix_attr_grp);
    return 0;
}

static void __exit sysfs_ctrl_exit(void)
{
    printk(KERN_INFO "sysfs_ctrl_exit\n");
    sysfs_remove_group(my_kobj, &nix_attr_grp);
    kobject_put(my_kobj);
}

/*
cat /sys/derek/nix_test/nix

sudo chmod 666 /sys/derek/nix_test/nix_status
echo off > /sys/derek/nix_test/nix_status
echo on > /sys/derek/nix_test/nix_status
cat /sys/derek/nix_test/nix_status
*/

module_init(sysfs_ctrl_init);
module_exit(sysfs_ctrl_exit);

MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  

