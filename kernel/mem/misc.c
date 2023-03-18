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

#include <linux/export.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <asm/cacheflush.h>
#include <asm/cache.h>
#include <asm/tlbflush.h>

#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of_dma.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>

#define MISC_NAME   "misc_cache"

#define DMA_BUFF_SZ 			(16 * 1024 * 1024)
struct dma_proxy_mem {
	unsigned char buffer[DMA_BUFF_SZ];
	unsigned int length;
    unsigned int phy_addr;
};

typedef struct
{
	void *buff;
	unsigned long len;
}IO_DATA_ST;

struct dma_proxy_mem *dma_mem_virt = NULL;
dma_addr_t dma_mem_phy = 0;

static struct device *cma_dev;  

static int misc_open(struct inode *inode, struct file *file)
{
    pr_info("misc_open\n");
    return 0;
}

static int misc_map(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;

    if (dma_mem_virt == NULL) return -1;
    printk("vm_start 0x%lx 0x%lx, vm_pgoff 0x%lx, vm_page_prot 0x%x \n", 
            vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_page_prot);

    ret = dma_mmap_coherent(cma_dev, vma,
     					   dma_mem_virt, dma_mem_phy,
     					   vma->vm_end - vma->vm_start);
    if (ret)
        printk("%s: dma_mmap_coherent failed at [0x%lx 0x%lx]\n",
            __func__, vma->vm_start, vma->vm_end);
    else
        printk("%s: map 0x%x to 0x%lx, size: 0x%lx\n", __func__, dma_mem_virt,
            vma->vm_start, vma->vm_end - vma->vm_start);

    return 0;
}

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	IO_DATA_ST temp_data;
	
	pr_info("misc_ioctl cmd 0x%x \n", cmd);
    switch(cmd)
    {
    #ifdef OP_CACHE_AREA
        case 0x100: //virt_addr, len
            if(copy_from_user(&temp_data,  (int *)arg, sizeof(IO_DATA_ST)))
                return -EFAULT;
			flush_user_va(&temp_data);
            break;
        case 0x200: //virt_addr, len
            if(copy_from_user(&temp_data,  (int *)arg, sizeof(IO_DATA_ST)))
                return -EFAULT;
            invalid_user_va(&temp_data);
            break;
    #endif

        case 0x300: //get phy address
            if(copy_to_user( (int *)arg, &dma_mem_phy, sizeof(dma_addr_t)))
                return -EFAULT;
            pr_info("check 0x%x \n", dma_mem_virt->buffer[0]);
            break;

		default:
			break;
    }
     
    return 0;
}
 
static const struct file_operations misc_fops =
{
    .owner  = THIS_MODULE,
    .open   = misc_open,
    .mmap   = misc_map,
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
    int ret, i, page_num;
     
    ret = misc_register(&misc_dev);
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }

    cma_dev = misc_dev.this_device;  
    cma_dev->coherent_dma_mask = ~0;  
    dma_mem_virt = dma_alloc_coherent(cma_dev, sizeof(struct dma_proxy_mem), &dma_mem_phy, GFP_KERNEL|GFP_DMA);  
	if (!dma_mem_virt) {
        pr_info("DMA allocation error\n");
        dma_mem_phy = 0;
	} else {
        page_num = sizeof(struct dma_proxy_mem)/PAGE_SIZE;
        pr_info("Allocating memory at va 0x%p, pa 0x%x, size %u(%d) \n", 
                dma_mem_virt, dma_mem_phy, sizeof(struct dma_proxy_mem), page_num);
	    dma_mem_virt->length = sizeof(struct dma_proxy_mem);
        dma_mem_virt->phy_addr = dma_mem_phy;
        for (i = 0; i < page_num; i++) {
            SetPageReserved(virt_to_page((char *)dma_mem_virt + PAGE_SIZE*i));
        }
    }

    return 0;
}

static void __exit misc_exit(void)
{
    int i;

	pr_info("misc_exit\n");
    if (dma_mem_virt) {
        for (i = 0; i < sizeof(struct dma_proxy_mem)/PAGE_SIZE; i++) {
            ClearPageReserved(virt_to_page((char *)dma_mem_virt + PAGE_SIZE*i));
        }
        dma_free_coherent(cma_dev, sizeof(struct dma_proxy_mem), dma_mem_virt, dma_mem_phy);  
    }

    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("derek");

