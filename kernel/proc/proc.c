
#include <linux/init.h>  
#include <linux/version.h>  
#include <linux/module.h>  
#include <linux/sched.h>  
#include <linux/uaccess.h>  
#include <linux/proc_fs.h>  
#include <linux/fs.h>  
#include <linux/seq_file.h>   
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/slab.h> /* kmalloc, kfree */
#include <linux/irq.h> /* IRQ_TYPE_EDGE_BOTH */
#include <linux/interrupt.h>
#include <asm/uaccess.h>  

 

#if 1

int intr_cnt = 0;

void my_tasklet_func(unsigned long data)
{
    printk("my_tasklet_func called\n");
    intr_cnt++;
}

DECLARE_TASKLET(xx_tasklet, my_tasklet_func, 0);

struct work_struct xx_wq;

void my_wq_func(struct work_struct *work)
{
    printk("my_wq_func called\n");
    intr_cnt += 100;
}

static int user_cmd_proc(char *user_cmd)
{
    printk("user_cmd_proc: %s", user_cmd);

    if(strncmp(user_cmd, "tasklet", 7) == 0) {
        tasklet_schedule(&xx_tasklet);
    }

    if(strncmp(user_cmd, "workqueue", 9) == 0) {
        schedule_work(&xx_wq);
    }
    
    return 0;
}




#endif


#if 1

#define MAX_CMD_LEN 256  
 
char global_buffer[MAX_CMD_LEN] = {0};

static int my_proc_show(struct seq_file *seq, void *v)
{
    printk("my_proc_show called\n");
    
    seq_printf(seq, "current kernel time is %ld\n", jiffies);  
    seq_printf(seq, "cmd: tasklet, workqueue \n");
    
    seq_printf(seq, "last user cmd: %s", global_buffer);

    return 0;        
}

static int my_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_proc_show, inode->i_private);
} 

static ssize_t my_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    printk("my_proc_write called, count=%d\n", count);
    
    if (count < 1 || count >= MAX_CMD_LEN ) {
        printk("invalid user cmd\n");
        return 0;
    }

    copy_from_user(global_buffer, buffer, count + 1);  
    user_cmd_proc(global_buffer);

    return count;
} 

struct file_operations proc_fops =
{
    .open  = my_proc_open,
    .read  = seq_read,
    .write  = my_proc_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

static struct proc_dir_entry *proc_dir = NULL;
static struct proc_dir_entry *proc_file = NULL; 

static int __init proc_test_init(void) 
{  
    proc_dir = proc_mkdir("my_proc", NULL);
    if (!proc_dir) {
         printk(KERN_DEBUG"proc_mkdir failed");
         return 0;
    }
    
    proc_file = proc_create("buffer", 0666, proc_dir, &proc_fops);
    if (!proc_file) {
         printk(KERN_DEBUG"proc_create failed");
         return 0;
    }

    //init workqueue
    INIT_WORK(&xx_wq, my_wq_func);
    
    return 0;  
}  
 
static void __exit proc_test_exit(void) 
{  
    remove_proc_entry("buffer", proc_dir);  
    remove_proc_entry("my_proc", NULL);  
}  

#endif

module_init(proc_test_init);  
module_exit(proc_test_exit);
MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  


/*
echo "Hello from kernel" > /proc/my_proc/buffer
cat /proc/my_proc/buffer

echo "tasklet" > /proc/my_proc/buffer
dmesg

echo "workqueue" > /proc/my_proc/buffer
dmesg

*/

