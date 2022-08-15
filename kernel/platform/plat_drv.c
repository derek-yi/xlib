#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
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
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h> 
 
static struct class *plat_leddrv_cls;
static struct device *plat_leddrvcls_device;
volatile unsigned long *gpfcon = NULL;
volatile unsigned long *gpfdat = NULL;
int major = 0;
int pin = 0;

#if xxx
static int plat_led_drv_open(struct inode *inode, struct file *file)
{
    *gpfcon &= ~(0x3<<(pin*2));
    *gpfcon |= (0x1<<(pin*2));
    
    return 0;
}

static ssize_t plat_led_drv_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
    int val;
    
    copy_from_user(&val, buf, count); //  copy_to_user();

    if (val == 1)
    {
        *gpfdat &= ~((1<<pin));
    }
    else
    {
        *gpfdat |= (1<<pin);
    }
    
    return 0;
}
#endif

#if 1

#define IO_CMD_LEN      256  
char user_cmd[IO_CMD_LEN] = {0};
char out_str[IO_CMD_LEN] = {0};

struct resource *res_info = NULL;

static int user_cmd_proc(char *user_cmd, char *out_str)
{
    if(strncmp(user_cmd, "sendsig", 7) == 0) {
        send_sig(SIGUSR1, current, 0); //send SIGUSR 1
    }
    
    if(strncmp(user_cmd, "showres", 7) == 0) {
        sprintf(out_str, "start=0x%x end=0x%x flags=0x%x\n", res_info->start, res_info->end, res_info->flags);
    }
    
    return 0;
}

int mem_open(struct inode *inode, struct file *filp)  
{  
    return 0;   
}  
  
int mem_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  

static long mem_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{    
    printk("mem_ioctl: %d \n", cmd);    
    
    if(copy_from_user(user_cmd,  (int *)arg, IO_CMD_LEN)) 
        return -EFAULT;
    
    printk("mem_ioctl: %s \n", user_cmd);    
    user_cmd_proc(user_cmd, out_str);
    
    if(copy_to_user( (int *)arg, out_str, IO_CMD_LEN)) 
        return -EFAULT;
    
    return 0;
}

#endif

static struct file_operations plat_led_drv_fops = {
    .owner  = THIS_MODULE,    
    .open   = mem_open, //plat_led_drv_open,    
    //.write  = plat_led_drv_write,   
    .unlocked_ioctl = mem_ioctl,
};

static int plat_led_drv_probe(struct platform_device *dev)
{
    struct resource * res;
    
    res_info = platform_get_resource(dev, IORESOURCE_MEM, 0);
    //gpfcon = (volatile unsigned long *)ioremap(res->start, res->end - res->start + 1);
    //gpfdat = gpfcon + 1;
    
    res = platform_get_resource(dev, IORESOURCE_IRQ, 0);
    pin = res->start;

    major = register_chrdev(0, "plat_led_drv", &plat_led_drv_fops);
    
    plat_leddrv_cls = class_create(THIS_MODULE, "plat_led_drv");
    plat_leddrvcls_device = device_create(plat_leddrv_cls, NULL, MKDEV(major, 0), NULL, "plat_led"); /* /dev/xyz */

    return 0;
}

static int plat_led_drv_remove(struct platform_device *dev)
{
    unregister_chrdev(major, "plat_led_drv");
    device_unregister(plat_leddrvcls_device);
    class_destroy(plat_leddrv_cls);
    iounmap(gpfcon);

    return 0;
}
 
static struct platform_driver plat_led_drv = {
    .driver = {
          .name = "plat_led_dev",
    },
    .probe = plat_led_drv_probe,
    .remove = plat_led_drv_remove,
};

static int plat_led_drv_init(void)
{
    platform_driver_register(&plat_led_drv);
    return 0;
}

static void plat_led_drv_exit(void)
{
    platform_driver_unregister(&plat_led_drv);
}

module_init(plat_led_drv_init);
module_exit(plat_led_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek");


