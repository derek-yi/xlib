
#include <linux/init.h>  
#include <linux/version.h>  
#include <linux/module.h>  
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
 
#define STRING_LEN 256  
 
char global_buffer[STRING_LEN] = {0};

#if 1

static irqreturn_t my_interrupt(int irq, void *dev_id)
{
	struct uio_info *info = (struct uio_info *)dev_id;
	unsigned long *ret_val_add = (unsigned long *)(info->mem[0].addr);
	*ret_val_add = 222;
	printk("my_interrupt: %d \n" ,(int)(*ret_val_add));

	return IRQ_RETVAL(IRQ_HANDLED);
}

struct uio_info kpart_info = {  
	.name = "kpart",
	.version = "0.1",
	.irq = 10, //unused
	.handler = my_interrupt, //unused
	.irq_flags = IRQ_TYPE_EDGE_RISING, //unused
};

static int drv_kpart_probe(struct device *dev);
static int drv_kpart_remove(struct device *dev);

static struct device_driver uio_dummy_driver = {  
	.name = "kpart",
	.bus = &platform_bus_type,
	.probe = drv_kpart_probe,
	.remove = drv_kpart_remove,
};

#ifdef HW_ENABLE
struct button_irq_desc {
    int irq;
    int num;
    char *name;    
};
 
static struct button_irq_desc button_irqs [] = {
    {8 , 1, "KEY0"},
    {11, 2, "KEY1"},
    {13, 3, "KEY2"},
    {14, 4, "KEY3"},
    {15, 5, "KEY4"},
};

static irqreturn_t buttons_interrupt(int irq, void *dev_id)
{
	struct button_irq_desc *button_irqs = (struct button_irq_desc *)dev_id;

	unsigned long *ret_val_add = (unsigned long *)(kpart_info.mem[1].addr);
	*ret_val_add = button_irqs->num;

	printk("%s is being pressed ..... \n", button_irqs->name);

	uio_event_notify(&kpart_info);

	return IRQ_RETVAL(IRQ_HANDLED);
}
#endif    

static int drv_kpart_probe(struct device *dev)
{  
    unsigned long *ret_val_addr;

	kpart_info.mem[0].addr = (unsigned long)kmalloc(1024, GFP_KERNEL);
	if(kpart_info.mem[0].addr == 0)
		return -ENOMEM;
	kpart_info.mem[0].memtype = UIO_MEM_LOGICAL;
	kpart_info.mem[0].size = 1024;

	ret_val_addr = (unsigned long *)(kpart_info.mem[0].addr);
	*ret_val_addr = 222;

	if(uio_register_device(dev, &kpart_info)){
		kfree((void *)kpart_info.mem[0].addr);
		return -ENODEV;
	}

#ifdef HW_ENABLE
	int i = 0 ,err = 0;
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
		err = request_irq(button_irqs[i].irq, buttons_interrupt, IRQ_TYPE_EDGE_RISING, 
			              button_irqs[i].name, (void *)&button_irqs[i]);
		if (err)
			break;
	}
#endif    

	return 0;
}  

static int drv_kpart_remove(struct device *dev)
{
	kfree((void *)kpart_info.mem[0].addr);
	uio_unregister_device(&kpart_info);
	return 0;
}

#endif

#if 1

static int user_cmd_proc(char *user_cmd)
{
    if(strncmp(user_cmd, "sendsig", 7) == 0) {
    	unsigned long *ret_val_add = (unsigned long *)(kpart_info.mem[0].addr);
    	*ret_val_add = 333;
        uio_event_notify(&kpart_info);
    }
    
    return 0;
}

static int my_proc_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, "current kernel time is %ld\n", jiffies);  
    seq_printf(seq, "last cmd: %s", global_buffer);

    return 0;        
}

static int my_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_proc_show, inode->i_private);
} 

static ssize_t my_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    if (count > 0) {
        printk("my_proc_write called\n");
        copy_from_user(global_buffer, buffer, count);  
        user_cmd_proc(global_buffer);
    }

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
static struct platform_device * uio_dummy_device;

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

    uio_dummy_device = platform_device_register_simple("kpart", -1, NULL, 0);
	driver_register(&uio_dummy_driver);   
    
    return 0;  
}  
 
static void __exit proc_test_exit(void) 
{  
    remove_proc_entry("buffer", proc_dir);  
    remove_proc_entry("my_proc", NULL);  

    platform_device_unregister(uio_dummy_device);
	driver_unregister(&uio_dummy_driver);    
}  
#endif
 
module_init(proc_test_init);  
module_exit(proc_test_exit);
MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  


/*
sudo modprobe uio
sudo insmod myuio.ko

cat /proc/my_proc/buffer
echo "sendsig" > /proc/my_proc/buffer
cat /proc/my_proc/buffer
*/

