
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
#include <linux/debugfs.h>

 
#define MISC_NAME   "miscdriver"

#if 0
struct dentry* debugfs_create_dir(const char *name, struct dentry *parent);
struct dentry *debugfs_create_u8(const char *name, mode_t mode, struct dentry *parent, u8 *value);
struct dentry *debugfs_create_u16(const char *name, mode_t mode, struct dentry *parent, u16 *value);
struct dentry *debugfs_create_u32(const char *name, mode_t mode, struct dentry *parent, u32 *value);
struct dentry *debugfs_create_u64(const char *name, mode_t mode, struct dentry *parent, u64 *value);
//struct dentry *debugfs_create_bool(const char *name, mode_t mode, struct dentry *parent, u32 *value);
struct dentry *debugfs_create_blob(const char *name, mode_t mode, struct dentry *parent, struct debugfs_blob_wrapper *blob);
void debugfs_remove(struct dentry *dentry);
#endif

struct dentry* dbg_root = NULL;

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

    dbg_root = debugfs_create_dir("misc", NULL);
    if (dbg_root == NULL)
    {
        printk("debugfs_create_dir error\n");
        return ret;
    }
    debugfs_create_u32("u32", 0660, dbg_root, &temp_data);
     
    return 0;
}
 
static void __exit misc_exit(void)
{
    misc_deregister(&misc_dev);
    debugfs_remove(dbg_root);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Decly");


/*

sudo insmod debugfs.ko
sudo cat /sys/kernel/debug/misc/u32

*/

