
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/errno.h> 
#include <linux/mm.h> 
#include <linux/cdev.h> 
#include <linux/miscdevice.h>
 
#define MISC_NAME   "miscdriver"
 
static int misc_open(struct inode *inode, struct file *file)
{
    printk("misc_open\n");
    return 0;
}
 
int temp_data = 0;
 
static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
    switch(cmd)
    {
        case 0x100:
            if(copy_from_user(&temp_data,  (int *)arg, sizeof(int)))
                return -EFAULT;
            break;
         
        case 0x101:
            if(copy_to_user( (int *)arg, &temp_data, sizeof(int)))
                return -EFAULT;
            break;
    }
     
    //printk(KERN_NOTICE"ioctl CMD%d done!\n",temp);   
    return 0;
}
 
 
static const struct file_operations misc_fops =
{
    .owner   =   THIS_MODULE,
    .open    =   misc_open,
    .unlocked_ioctl = misc_ioctl,
};
 
static struct miscdevice misc_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = MISC_NAME,
    .fops = &misc_fops,
};
 
 
static int __init misc_init(void)
{
    int ret;
     
    ret = misc_register(&misc_dev);
    if (ret)
    {
        printk("misc_register error\n");
        return ret;
    }
 
    return 0;
}
 
static void __exit misc_exit(void)
{
    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Decly");

