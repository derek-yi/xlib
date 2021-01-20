
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
 
char g_val[20] = "15";  
struct dentry *root_dentry = NULL;
struct dentry *sub_dentry = NULL;  
  
static ssize_t global_read(struct file *filp, char __user *buf, size_t len, loff_t *off)  
{  
    int ret;  
    char val[20];  
    
    printk(KERN_ERR "###### global_read \n");  
    sprintf(val, "%s\n", g_val);  
    
    ret = simple_read_from_buffer(buf, len, off, val, strlen(val));  
    return ret;  
}  
  
static ssize_t global_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)  
{  
    int  ret = 0;  
    char val[20];     
    int i =0, j = 0;  
    unsigned long p = *off;  
    unsigned int count = len;  
  
    memset(val, 0, 20);  
      
    if (p >= GLOBALMEM_SIZE)  
    {  
        return count ?  - ENXIO: 0;       
    }  
    if (count > GLOBALMEM_SIZE - p)  
    {  
        count = GLOBALMEM_SIZE - p;  
    }  
      
    if(copy_from_user(val, buf, count))  
    {  
        ret = -EFAULT;  
    }  
    else  
    {  
        *off += count;  
        ret = count;  
    }     
    memset(g_val, 0, 20);  
      
    for(i = 0; i < strlen(val); i++)  
    {  
        if(val[i] >= '0' && val[i] <= '9')  
        {  
            g_val[j] = val[i];  
            j++;  
        }  
    }     
    return ret;  
}  
  
struct file_operations fileops = {  
    .read = global_read,  
    .write = global_write,  
};  
  
static int __init globalvar_init(void)  
{  
    printk(KERN_ERR "golabvar_init \n");  
    
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
  	debugfs_remove(sub_dentry);
  	debugfs_remove(root_dentry); 
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
