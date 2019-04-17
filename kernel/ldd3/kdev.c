/****************************************************************************** 
*Name: memdev.c 
*Desc: 字符设备驱动程序的框架结构
*Parameter:  
*Return: 
*Author: derek 
*Date: 2013-6-4 
********************************************************************************/  
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
#include <linux/sched.h>

#define IO_CMD_LEN      256  
#define CHAR_DEV_NAME   "kdev"

#if 1

static int user_cmd_proc(char *user_cmd, char *out_str)
{
    if(strncmp(user_cmd, "sendsig", 7) == 0) {
        //send_sig(SIGUSR1, current, 0); //send SIGUSR 1
    }
    
    if(strncmp(user_cmd, "showpid", 7) == 0) {
        sprintf(out_str, "pid=%d tgid=%d\n", current->pid, current->tgid);
    }
    
    return 0;
}
#endif

#if 1
  
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

static long mem_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
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

    device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    return 0;  

failed:   
    cdev_del(&my_dev);
    unregister_chrdev_region(devno, 1);  
    return result;  
}  
  
static void memdev_exit(void)  
{  
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

