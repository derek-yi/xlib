#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/errno.h> 
#include <linux/mm.h> 
#include <linux/module.h>  
#include <linux/init.h>  
#include <asm/uaccess.h> 
#include <linux/debugfs.h> 
  
#define DEVICE_NAME         ("my_dev")  
#define NODE_NAME           ("node")

#define GLOBALMEM_SIZE      512   
 
char mem_buff[GLOBALMEM_SIZE];  

struct dentry *root_dentry = NULL;
struct dentry *sub_dentry = NULL;  

static int global_open(struct inode *inode, struct file *filp) 
{ 
    filp->private_data = inode->i_private;
    return 0; 
} 
    
static ssize_t global_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)  
{  
    printk("global_read: count %d ppos %d \n", count, *ppos);  
    if (count > GLOBALMEM_SIZE)   
        count = GLOBALMEM_SIZE;  
    
    if (copy_to_user(buf, mem_buff, count))   
        return -EFAULT;   
    
    //*ppos += count;  
    return count;  
}  
  
static ssize_t global_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)  
{  
    printk("global_write: count %d ppos %d \n", count, *ppos);  
    if (count > GLOBALMEM_SIZE)   
        count = GLOBALMEM_SIZE;  
    
    if(copy_from_user(mem_buff, buf, count))   
        return -EFAULT;   
    
    //*ppos += count;  
    return count; 
}  
  
struct file_operations fileops = {  
    .owner = THIS_MODULE,
    .open = global_open,
    .read = global_read,  
    .write = global_write,  
};  
  
static int __init globalvar_init(void)  
{  
    printk("golabvar_init \n");  
    
    root_dentry = debugfs_create_dir(DEVICE_NAME, NULL);
    if (root_dentry != NULL)
    {
    	sub_dentry = debugfs_create_file(NODE_NAME, 0660, root_dentry, NULL, &fileops);
    }
      
    return 0;  
}   
  
static void __exit globalvar_exit(void)  
{  
    printk("golabvar_exit \n");  
  	debugfs_remove_recursive(root_dentry);  
}  

MODULE_LICENSE("GPL");  
module_init(globalvar_init);  
module_exit(globalvar_exit);  


/*
cat /sys/kernel/debug/my_dev/node
1234

echo 100 > /sys/kernel/debug/my_dev/node

cat /sys/kernel/debug/my_dev/node
100

*/
