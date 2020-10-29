

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/module.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

static int hello_value;

static ssize_t hello_show(struct device *dev, struct device_attribute *attr, char *buf)
{
   return sprintf(buf, "%d\n", hello_value);
}

static ssize_t hello_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
   sscanf(buf, "%du", &hello_value);
   return len;
}

//static struct attribute hello_value_attribute = __ATTR(hello_value, 0660, hello_show, hello_store);

static DEVICE_ATTR(hello_value, 0660, hello_show, hello_store);


static struct kobject *helloworld_kobj;

static int __init helloworld_init(void)
{
   int retval;

   helloworld_kobj = kobject_create_and_add("helloworld", kernel_kobj);
   if (!helloworld_kobj)
             return -ENOMEM;

   retval = sysfs_create_file(helloworld_kobj, &dev_attr_hello_value.attr);
   if (retval)
      kobject_put(helloworld_kobj);

   return retval;
}

static void __exit helloworld_exit(void)
{
   kobject_put(helloworld_kobj);
}

module_init(helloworld_init);

module_exit(helloworld_exit);

MODULE_AUTHOR("derek yi");  
MODULE_LICENSE("GPL");  
  

