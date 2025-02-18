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
#include <linux/platform_device.h>
#include <linux/of_dma.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#define MISC_NAME   			"misc_mem"

#define DMA_BUFF_SZ 			(16 * 4096)

int mem_type = 0;

char *dma_mem_virt = NULL;
dma_addr_t dma_mem_phy = 0;

char *kmalloc_mem_virt = NULL;
dma_addr_t kmalloc_mem_phy = 0;

static char glb_mem_virt[DMA_BUFF_SZ];
dma_addr_t glb_mem_phy = 0;

static struct device *cma_dev;  

static int misc_open(struct inode *inode, struct file *file)
{
    pr_info("misc_open\n");
    return 0;
}

static int misc_map(struct file *filp, struct vm_area_struct *vma)
{
    int ret = -1;
	unsigned long pfn_start;
	char *virt_addr = NULL;

    printk("vm_start 0x%lx vm_end 0x%lx, vm_pgoff 0x%lx \n", 
            vma->vm_start, vma->vm_end, vma->vm_pgoff);

	if (mem_type == 0) {
		virt_addr = dma_mem_virt;
        pfn_start = dma_mem_phy;
	    ret = dma_mmap_coherent(cma_dev, vma,
	     					   dma_mem_virt, dma_mem_phy,
	     					   vma->vm_end - vma->vm_start);
	} else if (mem_type == 1) {
		virt_addr = kmalloc_mem_virt;
		pfn_start = virt_to_phys(kmalloc_mem_virt + vma->vm_pgoff) >> PAGE_SHIFT;
	    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, vma->vm_end - vma->vm_start, vma->vm_page_prot);
	} else if (mem_type == 2) {
		virt_addr = glb_mem_virt;
		pfn_start = virt_to_phys(glb_mem_virt + vma->vm_pgoff) >> PAGE_SHIFT;
	    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, vma->vm_end - vma->vm_start, vma->vm_page_prot);
	} 

    if (ret)
        printk("%s: dma_mmap_coherent failed at [0x%lx 0x%lx]\n",
            __func__, vma->vm_start, vma->vm_end);
    else
        printk("%s: map 0x%p(0x%lx) to 0x%lx, size 0x%lx\n", __func__, virt_addr, pfn_start,
            vma->vm_start, vma->vm_end - vma->vm_start);

    return ret;
}

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	dma_addr_t phy_addr = 0;
    char *v_addr;
	
	pr_info("misc_ioctl cmd 0x%x \n", cmd);
    switch(cmd)
    {
		case 0x100: //set mem_type
			if(copy_from_user(&mem_type,  (int *)arg, sizeof(int)))
				return -EFAULT;
			pr_info("mem_type %d \n", mem_type);
			break;
			
        case 0x200: //get phy address
        	if (mem_type == 0) {
				phy_addr = dma_mem_phy;
        	} else if (mem_type == 1) {
				phy_addr = kmalloc_mem_phy;
        	} else if (mem_type == 2) {
				phy_addr = glb_mem_phy;
        	} 
			
            if(copy_to_user( (int *)arg, &phy_addr, sizeof(dma_addr_t))) {
                return -EFAULT;
            }
            break;

        case 0x300:
        	if (mem_type == 0) {
				v_addr = dma_mem_virt;
        	} else if (mem_type == 1) {
        	    v_addr = kmalloc_mem_virt;
        	} else if (mem_type == 2) {
                v_addr = glb_mem_virt;
        	}
            pr_info("check 0x%x 0x%x\n", v_addr[0], v_addr[1]);
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
    int ret, page_num;
     
    ret = misc_register(&misc_dev);
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }

    cma_dev = misc_dev.this_device;  
    //cma_dev->coherent_dma_mask = ~0;
    cma_dev->coherent_dma_mask = 0xFFFFFFFF;
    dma_mem_virt = dma_alloc_coherent(cma_dev, DMA_BUFF_SZ, &dma_mem_phy, GFP_KERNEL);  
	if (!dma_mem_virt) {
        pr_info("DMA allocation error\n");
        dma_mem_phy = 0;
	} else {
        page_num = DMA_BUFF_SZ/PAGE_SIZE;
        pr_info("DMA memory at va 0x%p, pa 0x%llx, size %u(%d) \n", 
                dma_mem_virt, dma_mem_phy, DMA_BUFF_SZ, page_num);
    }

	kmalloc_mem_virt = kzalloc(DMA_BUFF_SZ, GFP_KERNEL);
	if (!kmalloc_mem_virt) {
        pr_info("kzalloc allocation error\n");
	} else {
		kmalloc_mem_phy = virt_to_phys(kmalloc_mem_virt);
        pr_info("kzalloc memory at va 0x%p, pa 0x%llx, size %u \n", 
                kmalloc_mem_virt, kmalloc_mem_phy, DMA_BUFF_SZ);
	}

	glb_mem_phy = virt_to_phys(glb_mem_virt);
    memset(glb_mem_virt, 0, DMA_BUFF_SZ);
	pr_info("glb memory at va 0x%p, pa 0x%llx, size %u \n", 
			glb_mem_virt, glb_mem_phy, DMA_BUFF_SZ);

    return 0;
}

static void __exit misc_exit(void)
{
	pr_info("misc_exit\n");
	
    if (dma_mem_virt) {
        dma_free_coherent(cma_dev, DMA_BUFF_SZ, dma_mem_virt, dma_mem_phy);  
    }

	if (kmalloc_mem_virt) {
		kfree(kmalloc_mem_virt);
	}

    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("derek");

