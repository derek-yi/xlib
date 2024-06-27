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

char *buf = NULL;  

#define MM_SIZE 4096 

static int misc_open(struct inode *inode, struct file *file)
{
    printk("misc_open \n");  
    return 0;
}

static int misc_close(struct inode *indoe, struct file *file)  
{  
    printk("misc_close \n");  
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
     
    return 0;
}

//sudo cat /sys/devices/virtual/misc/miscdriver/ext_sync
static ssize_t ext_sync_show(struct device *dev, struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "buff %d %d %d %d %d, temp_data %d \n", 
                   buf[0], buf[1], buf[2], buf[3], buf[4], temp_data);
}

static int device_mmap(struct file *file, struct vm_area_struct *vma)  
{  
    printk("device_mmap\n");  
    
    vma->vm_flags |= VM_IO; 
    if(remap_pfn_range(vma,
                       vma->vm_start,
                       virt_to_phys(buf)>>PAGE_SHIFT,
                       vma->vm_end - vma->vm_start,
                       vma->vm_page_prot))
    {  
        return -EAGAIN;  
    }  
    
    return 0;  
}  

static DEVICE_ATTR(ext_sync, 0660, ext_sync_show, NULL);
 
static const struct file_operations misc_fops =
{
    .owner      = THIS_MODULE,
    .open       = misc_open,
    .release    = misc_close,
    .mmap       = device_mmap,  
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
    int ret, i;
     
    ret = misc_register(&misc_dev);
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }

    device_create_file(misc_dev.this_device, &dev_attr_ext_sync);

    buf = (char *)kmalloc(MM_SIZE, GFP_KERNEL);
    if (buf == NULL) {
        printk("kmalloc failed\n");
        return -1;
    }
    //SetPageReserved(virt_to_page(buf)); //no need
    for (i = 0; i < 1024; i++) {
        buf[i] = i;
    }

    return 0;
}
 
static void __exit misc_exit(void)
{
    if (buf) kfree(buf);     
    device_remove_file(misc_dev.this_device, &dev_attr_ext_sync);
    misc_deregister(&misc_dev);
}

module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek");

