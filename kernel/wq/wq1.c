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
#include <linux/interrupt.h>  
#include <linux/workqueue.h>

static int nix_status = 0;

static struct kobject *my_kobj;

struct workqueue_struct *my_wq;

struct work_struct my_work;

static void mywork_func(struct work_struct *work)
{
    printk("%s called: \n",__FUNCTION__);
    msleep(10);
    printk("%s end \n",__FUNCTION__);
}

static ssize_t nix_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "nix_show ro \n");
}

static ssize_t nix_status_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "nix_status: %d \n", nix_status);
}

static ssize_t nix_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    printk(KERN_INFO "nix_status_store: %s \n", buf);
    if (0 == memcmp(buf, "on", 2)) {
        //1 user workque
        queue_work(my_wq, &my_work);
        nix_status = 1;
    } 
    else if (0 == memcmp(buf, "off", 3)) {
        //2 system workque
        //schedule_work => queue_work(keventd_wq, work)
        schedule_work(&my_work); 
        //schedule_delayed_work(&my_queue_work, tick); 
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
    int ret;
    
    my_kobj = kobject_create_and_add("derek", kernel_kobj->parent);
    ret = sysfs_create_group(my_kobj, &nix_attr_grp);
    printk(KERN_INFO "sysfs_ctrl_init ret %d \n", ret);

    my_wq = create_workqueue("my_wq"); //1 user workque
    INIT_WORK(&my_work, mywork_func);
    
    return 0;
}

static void __exit sysfs_ctrl_exit(void)
{
    printk(KERN_INFO "sysfs_ctrl_exit \n");
    destroy_workqueue(my_wq);
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

