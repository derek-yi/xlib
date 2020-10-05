
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
    buf = (char *)kmalloc(MM_SIZE, GFP_KERNEL);
    if(buf == NULL) {
        printk("kmalloc failed\n");
        return -1;
    }
    
    return 0;
}

static int misc_close(struct inode *indoe, struct file *file)  
{  
    printk("misc_close\n");  
    if(buf)  
    {  
        kfree(buf);  
    }  
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

static int device_mmap(struct file *file, struct vm_area_struct *vma)  
{  
    printk("device_mmap\n");  
    
    vma->vm_flags |= VM_IO;//表示对设备IO空间的映射  
    //vma->vm_flags |= VM_RESERVED;//标志该内存区不能被换出，在设备驱动中虚拟页和物理页的关系应该是长期的，应该保留起来，不能随便被别的虚拟页换出 
    
    if(remap_pfn_range(vma,//虚拟内存区域，即设备地址将要映射到这里  
                       vma->vm_start,//虚拟空间的起始地址  
                       virt_to_phys(buf)>>PAGE_SHIFT,//与物理内存对应的页帧号，物理地址右移12位  
                       vma->vm_end - vma->vm_start,//映射区域大小，一般是页大小的整数倍  
                       vma->vm_page_prot))//保护属性，  
    {  
        return -EAGAIN;  
    }  
    
    return 0;  
}  

 
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

