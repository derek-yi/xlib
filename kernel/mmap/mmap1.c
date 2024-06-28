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
#include <linux/dma-mapping.h>

#define MISC_NAME       "misc_xx"
#define MEM_SIZE        4096
//#define DMA_MAP_MEM

char *map_mem = NULL;  

static struct device *cma_dev;  

#ifdef DMA_MAP_MEM
dma_addr_t dma_mem_phy = 0;
#endif

static int misc_open(struct inode *inode, struct file *file)
{
#if 0
    struct mm_struct *mm = current->mm;

    printk("client: %s (%d)\n", current->comm, current->pid);
    printk("code  section: [0x%lx   0x%lx]\n", mm->start_code, mm->end_code);
    printk("data  section: [0x%lx   0x%lx]\n", mm->start_data, mm->end_data);
    printk("brk   section: s: 0x%lx, c: 0x%lx\n", mm->start_brk, mm->brk);
    printk("mmap  section: s: 0x%lx\n", mm->mmap_base);
    printk("stack section: s: 0x%lx\n", mm->start_stack);
    printk("arg   section: [0x%lx   0x%lx]\n", mm->arg_start, mm->arg_end);
    printk("env   section: [0x%lx   0x%lx]\n", mm->env_start, mm->env_end);
#endif
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
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
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
    printk("misc_ioctl\n");  
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
    int ret, i;

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
        return -1;
	} else {
        pr_info("DMA memory at va 0x%p, pa 0x%llx, size %u \n", 
                map_mem, dma_mem_phy, MEM_SIZE);
    }
#else
    map_mem = (char *)kmalloc(MEM_SIZE, GFP_KERNEL);
    if (map_mem == NULL) {
        printk("kmalloc failed\n");
        return -1;
    }
    SetPageReserved(virt_to_page(map_mem));
    pr_info("kmalloc memory at va 0x%p, pa 0x%llx, size %u \n", 
            map_mem, virt_to_phys(map_mem), MEM_SIZE);
#endif

    for (i = 0; i < 100; i++) {
        map_mem[i] = i;
    }

    return 0;
}

static void __exit misc_exit(void)
{
	pr_info("misc_drv exit \n");
#ifdef DMA_MAP_MEM
    if (map_mem) {
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

