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
#include <linux/sched.h> 

#define MISC_NAME       "misc_xx"
#define MEM_SIZE        40960
#define DMA_MAP_MEM

typedef struct
{
	void *buff;
	unsigned long len;
    unsigned long param1;
    unsigned long param2;
}IO_DATA_ST;

char *map_mem = NULL;  

static struct device *cma_dev;  

#ifdef DMA_MAP_MEM
dma_addr_t dma_mem_phy = 0;
#endif

static int misc_open(struct inode *inode, struct file *file)
{
    struct mm_struct *mm = current->mm;
    
    printk("client: %s (%d)\n", current->comm, current->pid);
    printk("code  section: [0x%lx   0x%lx]\n", mm->start_code, mm->end_code);
    printk("data  section: [0x%lx   0x%lx]\n", mm->start_data, mm->end_data);
    printk("brk   section: s: 0x%lx, c: 0x%lx\n", mm->start_brk, mm->brk);
    printk("mmap  section: s: 0x%lx\n", mm->mmap_base);
    printk("stack section: s: 0x%lx\n", mm->start_stack);
    printk("arg   section: [0x%lx   0x%lx]\n", mm->arg_start, mm->arg_end);
    printk("env   section: [0x%lx   0x%lx]\n", mm->env_start, mm->env_end);
    return 0;
}

static int misc_close(struct inode *indoe, struct file *file)  
{  
    printk("misc_close\n");  
    return 0;  
}

static int misc_map(struct file *filp, struct vm_area_struct *vma)
{
#ifdef DMA_MAP_MEM
    dma_mmap_coherent(cma_dev, vma,
                      map_mem, dma_mem_phy,
                      vma->vm_end - vma->vm_start);

#else
    vma->vm_flags |= VM_IO; //keep coherent
    //vma->vm_flags |= VM_RESERVED;
    if (remap_pfn_range(vma,
                       vma->vm_start,
                       virt_to_phys(map_mem)>>PAGE_SHIFT,
                       vma->vm_end - vma->vm_start,
                       vma->vm_page_prot))
    {  
        return -EAGAIN;  
    }  
#endif
    return 0;
}

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	IO_DATA_ST temp_data;
	
    switch(cmd)
    {
        case 0x500:
            if(copy_from_user(&temp_data,  (int *)arg, sizeof(IO_DATA_ST)))
                return -EFAULT;
			//stop_pid((int)temp_data.param);
            break;

		default:
			break;
    }
     
    return 0;
}
 
static const struct file_operations misc_fops =
{
    .owner      = THIS_MODULE,
    .open       = misc_open,
    .release    = misc_close,
    .mmap       = misc_map,
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

	pr_info("misc_drv init \n");
    ret = misc_register(&misc_dev);
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }

    cma_dev = misc_dev.this_device;

#ifdef DMA_MAP_MEM
    //cma_dev->coherent_dma_mask = ~0;
    cma_dev->coherent_dma_mask = 0xFFFFFFFF;
    map_mem = dma_alloc_coherent(cma_dev, MEM_SIZE, &dma_mem_phy, GFP_KERNEL);  
	if (!map_mem) {
        pr_info("DMA allocation error\n");
        dma_mem_phy = 0;
	} else {
        pr_info("DMA memory at va 0x%p, pa 0x%llx, size %u \n", 
                map_mem, dma_mem_phy, DMA_BUFF_SZ);
    }
#else
    map_mem = (char *)kmalloc(MEM_SIZE, GFP_KERNEL);
    if (map_mem == NULL) {
        printk("kmalloc failed\n");
        return -1;
    }
    memset(map_mem, 0, MEM_SIZE);
#endif

    return 0;
}

static void __exit misc_exit(void)
{
	pr_info("misc_drv exit \n");
#ifdef DMA_MAP_MEM
    if (dma_mem_virt) {
        dma_free_coherent(cma_dev, MEM_SIZE, map_mem, dma_mem_phy);  
    }
#else
    if (map_mem) kfree(map_mem);   
#endif

    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("derek");

