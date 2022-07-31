/****************************************************************************** 
* Name  : memdev.c 
* Desc  : xx
* Author: derek 
* Date  : 2013-6-4 
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
  
#define MEMDEV_MAJOR            0 //260
#define MEMDEV_NR_DEVS          4
#define MEMDEV_SIZE             4096  
#define CHAR_DEV_NAME           "memdev"
  
struct mem_dev   
{   
    char data[MEMDEV_SIZE];   
    unsigned long size;   
};  
  
static int mem_major = MEMDEV_MAJOR;  
  
struct mem_dev *mem_devp = NULL;
  
struct cdev cdev;   

module_param(mem_major, int, S_IRUGO);  

int mem_open(struct inode *inode, struct file *filp)  
{  
    struct mem_dev *dev;  
      
    int num = MINOR(inode->i_rdev);  
    if (num >= MEMDEV_NR_DEVS)   
        return -ENODEV;  
    
    dev = &mem_devp[num];  
    filp->private_data = dev;  
      
    return 0;   
}  
  
int mem_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
  
static ssize_t mem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)  
{  
    unsigned long p = *ppos;  
    unsigned int count = size;  
    int ret = 0;  
    struct mem_dev *dev = filp->private_data;

    if (p >= MEMDEV_SIZE)  
        return 0;  
    
    if (count > MEMDEV_SIZE - p) {
        count = MEMDEV_SIZE - p;  
    }

    if (copy_to_user(buf, (void*)(dev->data + p), count))  {  
        ret = - EFAULT;  
    } else {  
        *ppos += count;  
        ret = count;  
        printk(KERN_INFO "read %d bytes(s) from %d\n", count, p);  
    }  

    return ret;  
}  
  
static ssize_t mem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)  
{  
    unsigned long p = *ppos;  
    unsigned int count = size;  
    int ret = 0;  
    struct mem_dev *dev = filp->private_data;

    if (p >= MEMDEV_SIZE)  
        return 0;  
    
    if (count > MEMDEV_SIZE - p)  
        count = MEMDEV_SIZE - p;  

    if (copy_from_user(dev->data + p, buf, count)) {
        ret = - EFAULT;  
    } else {  
        *ppos += count;  
        ret = count;  
        printk(KERN_INFO "written %d bytes(s) from %d\n", count, p);  
    }  

    return ret;  
}  
  
static loff_t mem_llseek(struct file *filp, loff_t offset, int whence)  
{   
    loff_t newpos;  
  
    switch(whence) {  
      case 0: /* SEEK_SET */  
        newpos = offset;  
        break;  
  
      case 1: /* SEEK_CUR */  
        newpos = filp->f_pos + offset;  
        break;  
  
      case 2: /* SEEK_END */  
        newpos = MEMDEV_SIZE - 1 + offset;  
        break;  
  
      default: /* can't happen */  
        return -EINVAL;  
    }  
    
    if ((newpos<0) || (newpos>MEMDEV_SIZE))  
     return -EINVAL;  
       
    filp->f_pos = newpos;  
    return newpos;  
}  
  
static const struct file_operations mem_fops =  
{  
    .owner      = THIS_MODULE,  
    .llseek     = mem_llseek,  
    .read       = mem_read,  
    .write      = mem_write,  
    .open       = mem_open,  
    .release    = mem_release,  
};  

struct class *pclass;  

static int memdev_init(void)  
{  
    int result;  
    int i;  

    dev_t devno = MKDEV(mem_major, 0);  
    if (mem_major)  {
        result = register_chrdev_region(devno, MEMDEV_NR_DEVS, CHAR_DEV_NAME);  
    } else {
        result = alloc_chrdev_region(&devno, 0, MEMDEV_NR_DEVS, CHAR_DEV_NAME);  
        mem_major = MAJOR(devno);  
    }   
    if (result < 0)  
        return result;  

    printk("mem_major %d\n", mem_major);  
    cdev_init(&cdev, &mem_fops);  
    cdev.owner = THIS_MODULE;  
    cdev.ops = &mem_fops;  
    cdev_add(&cdev, MKDEV(mem_major, 0), MEMDEV_NR_DEVS);  

    mem_devp = kmalloc(MEMDEV_NR_DEVS * sizeof(struct mem_dev), GFP_KERNEL);  
    if (!mem_devp) {  
        result = - ENOMEM;  
        goto fail_malloc;  
    }  
    memset(mem_devp, 0, sizeof(struct mem_dev));  

    for (i=0; i < MEMDEV_NR_DEVS; i++) {  
        mem_devp[i].size = MEMDEV_SIZE;  
        memset(mem_devp[i].data, 0, MEMDEV_SIZE);  
    }  

    pclass = class_create(THIS_MODULE, CHAR_DEV_NAME);  
    if (IS_ERR(pclass)) {  
        printk("class_create failed!/n");  
        kfree(mem_devp);
        goto fail_malloc;  
    }  

    device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    return 0;  

fail_malloc:   
    unregister_chrdev_region(devno, MEMDEV_NR_DEVS);  
    return result;  
}  
  
static void memdev_exit(void)  
{  
    device_destroy(pclass, MKDEV(mem_major, 0));  
    class_destroy(pclass);
    cdev_del(&cdev);
    kfree(mem_devp);
    unregister_chrdev_region(MKDEV(mem_major, 0), MEMDEV_NR_DEVS);
}  
  
MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  
  
module_init(memdev_init);  
module_exit(memdev_exit);  

