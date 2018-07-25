/****************************************************************************** 
*Name: memdev.c 
*Desc: 字符设备驱动程序的框架结构
*Parameter:  
*Return: 
*Author: derek 
*Date: 2013-6-4 
********************************************************************************/  
#include <linux/init.h>  
#include <linux/version.h>  
#include <linux/module.h>  
#include <linux/mm.h>  
#include <linux/cdev.h>  
#include <linux/sched.h>
#include <linux/uaccess.h>  
#include <linux/proc_fs.h>  
#include <linux/fs.h>  
#include <linux/seq_file.h>   
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <asm/io.h>
#include <linux/slab.h> /* kmalloc, kfree */
#include <linux/irq.h> /* IRQ_TYPE_EDGE_BOTH */
#include <asm/uaccess.h>  


#if 1

//irqreturn_t (*handler)(int irq, struct uio_info *dev_info);
static irqreturn_t my_interrupt(int irq, struct uio_info *dev_id)
{
	struct uio_info *info = (struct uio_info *)dev_id;

    disable_irq_nosync(info->irq);
    
    uio_event_notify(info);

	return IRQ_RETVAL(IRQ_HANDLED);
}

static int irq_control(struct uio_info *info, s32 irq_on)
{
	if(irq_on) 
        enable_irq(info->irq);
    else 
        disable_irq_nosync(info->irq);

    return 0;
}

struct uio_info irq_info = {  
	.name = "uio_irq",
	.version = "0.1",
	.irq = 10, 
	.handler = my_interrupt,
	.irq_flags = IRQ_TYPE_EDGE_RISING, 
    .irqcontrol = irq_control, 
};
#endif

#if 1

#define IO_CMD_LEN      256  
#define CHAR_DEV_NAME   "kdev"

static int user_cmd_proc(char *user_cmd, char *out_str)
{
    if(strncmp(user_cmd, "sendsig", 7) == 0) {
        uio_event_notify(&irq_info);
        sprintf(out_str, "send ok\n");
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

char user_cmd[IO_CMD_LEN] = {0};
char out_str[IO_CMD_LEN] = {0};

static int mem_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{    
    printk("mem_ioctl: %d \n", cmd);    
    
    if(copy_from_user(user_cmd,  (int *)arg, IO_CMD_LEN)) 
        return -EFAULT;
    
    user_cmd_proc(user_cmd, out_str);
    
    if(copy_to_user( (int *)arg, out_str, IO_CMD_LEN)) 
        return -EFAULT;
    
    return 0;
}

static int mem_major = 0;
struct class *pclass = NULL;  
struct cdev my_dev;   
struct device *pdev;   

static const struct file_operations mem_fops =  
{  
    .owner = THIS_MODULE,  
    .unlocked_ioctl = mem_ioctl,
    .open = mem_open,  
    .release = mem_release,  
};  

static int memdev_init(void)  
{  
    int result;  

    dev_t devno = MKDEV(mem_major, 0);  

    if (mem_major) { /* 静态申请设备号*/  
        result = register_chrdev_region(devno, 2, CHAR_DEV_NAME);  
    } else { /* 动态分配设备号 */  
        result = alloc_chrdev_region(&devno, 0, 2, CHAR_DEV_NAME);  
        mem_major = MAJOR(devno);  
    }   

    if (result < 0)  {  
        printk("alloc_chrdev failed!\n");  
        return result;  
    }  

    cdev_init(&my_dev, &mem_fops);  
    my_dev.owner = THIS_MODULE;  
    my_dev.ops = &mem_fops;  
    cdev_add(&my_dev, MKDEV(mem_major, 0), 2);   /*设备数2*/  

    pclass = class_create(THIS_MODULE, CHAR_DEV_NAME);  
    if (IS_ERR(pclass))  {  
        printk("class_create failed!\n");  
        goto failed;  
    }  

    pdev = device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    uio_register_device(pdev, &irq_info); //todo
    
    return 0;  

failed:   
    cdev_del(&my_dev);
    unregister_chrdev_region(devno, 1);  
    return result;  
}  
  
static void memdev_exit(void)  
{  
    uio_unregister_device(&irq_info); //todo
    device_destroy(pclass, MKDEV(mem_major, 0));  
    class_destroy(pclass);
    
    cdev_del(&my_dev);
    unregister_chrdev_region(MKDEV(mem_major, 0), 2); 
}  
#endif
  
MODULE_AUTHOR("derek yi");  
MODULE_LICENSE("GPL");  
  
module_init(memdev_init);  
module_exit(memdev_exit);  

