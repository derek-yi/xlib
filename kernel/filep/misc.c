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
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#define MISC_NAME   "miscdriver"

#define XLOG_BUFF_MAX           512

#define MY_FILE "/tmp/kern.xlog"

int cnt = 0;

#if 0 //old kernel
int kern_xlog(const char *format, ...)
{
    struct file *log_fd = NULL;
    mm_segment_t old_fs;
    va_list args;
    char buf[XLOG_BUFF_MAX];
    int len;

    va_start(args, format);
    len = vsnprintf(buf, XLOG_BUFF_MAX, format, args);
    va_end(args);

    log_fd = filp_open(MY_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
    if ( IS_ERR(log_fd) )  {
        printk("failed to open file %s\n", MY_FILE);
        return 0;
    }

    //printk("write file %s\n", MY_FILE);
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    vfs_write(log_fd, buf, strlen(buf) + 1,  0);
    set_fs(old_fs);
    
    filp_close(log_fd, NULL);  
    log_fd = NULL;

    return len;    
}

#else

int kern_xlog(const char *format, ...)
{
    struct file *log_fd = NULL;
    loff_t pos = 0;
    va_list args;
    char buf[XLOG_BUFF_MAX];
    int len;

    log_fd = filp_open(MY_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
    if ( IS_ERR(log_fd) )  {
        printk("failed to open file %s\n", MY_FILE);
        return 0;
    }

    va_start(args, format);
    len = vsnprintf(buf, XLOG_BUFF_MAX, format, args);
    va_end(args);

    //kernel_read(i_fp, in_buf, count1, &pos);
    //printk("write file %s\n", MY_FILE);
    kernel_write(log_fd, buf, len, &pos);
    
    filp_close(log_fd, NULL);  
    return len;    
}

#endif

static int misc_open(struct inode *inode, struct file *file)
{
    printk("misc_open \n");
    kern_xlog("misc_open, cnt %d \n", cnt++);     
    return 0;
}
 
int temp_data = 0;
 
static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
    printk("misc_ioctl cmd %d \n", cmd);   
    kern_xlog("misc_ioctl, cnt %d \n", cnt++);     
    
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
    if (ret) {
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

