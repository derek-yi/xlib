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

//#define OP_CACHE_AREA

#define MISC_NAME   			"misc_op_cache"
#define DMA_BUFF_SZ 			(16 * 4096)

typedef struct
{
	void *buff; //dma_addr_t
	unsigned long len;
}IO_DATA_ST;

int mem_type = 0;

char *dma_mem_virt = NULL;
dma_addr_t dma_mem_phy = 0;

char *kmalloc_mem_virt = NULL;
dma_addr_t kmalloc_mem_phy = 0;

char glb_mem_virt[DMA_BUFF_SZ];
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

    printk("vm_start 0x%lu 0x%lx, vm_pgoff 0x%lx \n", 
            vma->vm_start, vma->vm_end, vma->vm_pgoff);

	if (mem_type == 0) {
		virt_addr = dma_mem_virt;
	    ret = dma_mmap_coherent(cma_dev, vma,
	     					   dma_mem_virt, dma_mem_phy,
	     					   vma->vm_end - vma->vm_start);
	} else if (mem_type == 1) {
		virt_addr = kmalloc_mem_virt;
		pfn_start = (virt_to_phys(kmalloc_mem_virt) >> PAGE_SHIFT) + vma->vm_pgoff;
	    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, vma->vm_end - vma->vm_start, vma->vm_page_prot);
	} else if (mem_type == 2) {
		virt_addr = glb_mem_virt;
		pfn_start = (virt_to_phys(glb_mem_virt) >> PAGE_SHIFT) + vma->vm_pgoff;
	    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, vma->vm_end - vma->vm_start, vma->vm_page_prot);
	} 

    if (ret)
        printk("%s: dma_mmap_coherent failed at [0x%lx 0x%lx]\n",
            __func__, vma->vm_start, vma->vm_end);
    else
        printk("%s: map 0x%p to 0x%lx, size 0x%lx\n", __func__, virt_addr,
            vma->vm_start, vma->vm_end - vma->vm_start);

    return ret;
}

#ifdef OP_CACHE_AREA

int flush_user_va(IO_DATA_ST *iocb)
{
	unsigned long len = iocb->len;
	char *buf = iocb->buff;
	struct page *pages[64];
	unsigned int pg_off = offset_in_page(buf);
	unsigned int pages_nr = (len + pg_off + PAGE_SIZE - 1) >> PAGE_SHIFT;
	int i;
	int rv;

	if ( (pages_nr == 0) || (pages_nr > 64) ) {
		pr_err("pages_nr %u", pages_nr);
		return -EINVAL;
	}

	/*pages = kmalloc(pages_nr * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("sgl allocation failed for %u pages", pages_nr);
		return -ENOMEM;
	}*/
	
	memset(pages, 0, pages_nr * sizeof(struct page *));
	rv = get_user_pages_fast((unsigned long)buf, pages_nr, 1/* write */, pages);
	//rv = get_user_pages((unsigned long)buf, pages_nr, FOLL_WRITE, pages, NULL);
	if (rv < 0) {
		pr_err("unable to pin down %u user pages, %d.\n", pages_nr, rv);
		goto err_out;
	}
	
	/* Less pages pinned than wanted */
	if (rv != pages_nr) {
		pr_err("unable to pin down all %u user pages, %d.\n", pages_nr, rv);
		rv = -EFAULT;
		goto err_out;
	}

	for (i = 1; i < pages_nr; i++) {
		if (pages[i - 1] == pages[i]) {
			pr_err("duplicate pages, %d, %d.\n", i - 1, i);
			rv = -EFAULT;
			goto err_out;
		}
	}

	//pr_info("buf 0x%p, pages_nr %u, page[0] 0x%p \n", buf, pages_nr, page_address(pages[0]));
	for (i = 0; i < pages_nr; i++) {
		struct page *pg = pages[i];

		//phys_addr_t phy_addr = virt_to_phys(page_address(pg));
		//pr_info("phy_addr 0x%llx, page_address 0x%p \n", phy_addr, page_address(pg));
		//flush_dcache_page(pg);
		__flush_dcache_area(page_address(pg), PAGE_SIZE);
	}

err_out:
	//kfree(pages);
	for (i = 0; i < pages_nr; i++) {
		if(pages[i]) put_page(pages[i]);
	}

	return 0;
}

int invalid_user_va(IO_DATA_ST *iocb)
{
	unsigned long len = iocb->len;
	char *buf = iocb->buff;
	struct page *pages[64];
	unsigned int pg_off = offset_in_page(buf);
	unsigned int pages_nr = (len + pg_off + PAGE_SIZE - 1) >> PAGE_SHIFT;
	int i;
	int rv;

	if ( (pages_nr == 0) || (pages_nr > 64) ) {
		pr_err("pages_nr %u", pages_nr);
		return -EINVAL;
	}

	/*pages = kmalloc(pages_nr * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("sgl allocation failed for %u pages", pages_nr);
		return -ENOMEM;
	}*/
	
	memset(pages, 0, pages_nr * sizeof(struct page *));
	rv = get_user_pages_fast((unsigned long)buf, pages_nr, 1/* write */, pages);
	if (rv < 0) {
		pr_err("unable to pin down %u user pages, %d.\n", pages_nr, rv);
		goto err_out;
	}
	
	/* Less pages pinned than wanted */
	if (rv != pages_nr) {
		pr_err("unable to pin down all %u user pages, %d.\n", pages_nr, rv);
		rv = -EFAULT;
		goto err_out;
	}

	for (i = 1; i < pages_nr; i++) {
		if (pages[i - 1] == pages[i]) {
			pr_err("duplicate pages, %d, %d.\n", i - 1, i);
			rv = -EFAULT;
			goto err_out;
		}
	}

	//pr_err("buf 0x%p, pages_nr %u, page_address 0x%p \n", buf, pages_nr, page_address(pages[0]));
	for (i = 0; i < pages_nr; i++) {
		struct page *pg = pages[i];
		__inval_dcache_area(page_address(pg), PAGE_SIZE);
	}

err_out:
	//kfree(pages);
	for (i = 0; i < pages_nr; i++) {
		if(pages[i]) put_page(pages[i]);
	}

	return 0;
}

int flush_user_pa(IO_DATA_ST *iocb)
{
	void *virt_addr = phys_to_virt((unsigned long)iocb->buff);
	__flush_dcache_area(virt_addr, iocb->len);
	return 0;
}

int invalid_user_pa(IO_DATA_ST *iocb)
{
	void *virt_addr = phys_to_virt((unsigned long)iocb->buff);
	__inval_dcache_area(virt_addr, iocb->len);
	return 0;
}

#endif

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	dma_addr_t phy_addr = 0;
	#ifdef OP_CACHE_AREA
	IO_DATA_ST temp_data;
	#endif
	
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
		case 0x102: //phy_addr, len
			if(copy_from_user(&temp_data,  (int *)arg, sizeof(IO_DATA_ST)))
				return -EFAULT;
			flush_user_pa(&temp_data);
			break;
		case 0x103: //phy_addr, len
			if(copy_from_user(&temp_data,  (int *)arg, sizeof(IO_DATA_ST)))
				return -EFAULT;
			invalid_user_pa(&temp_data);
			break;
    #endif

		case 0x300: //set mem_type
			if(copy_from_user(&mem_type,  (int *)arg, sizeof(int)))
				return -EFAULT;
			pr_info("mem_type %d \n", mem_type);
			break;
			
        case 0x301: //get phy address
        	if (mem_type == 0) {
				phy_addr = dma_mem_phy;
				pr_info("check 0x%x \n", dma_mem_virt[0]);
        	} else if (mem_type == 1) {
				phy_addr = kmalloc_mem_phy;
				pr_info("check 0x%x \n", kmalloc_mem_virt[0]);
        	} else if (mem_type == 2) {
				phy_addr = glb_mem_phy;
				pr_info("check 0x%x \n", glb_mem_virt[0]);
        	} 
			
            if(copy_to_user( (int *)arg, &phy_addr, sizeof(dma_addr_t))) {
                return -EFAULT;
            }
			
        	if (mem_type == 0) {
				dma_mem_virt[0] = 0x10;
        	} else if (mem_type == 1) {
				kmalloc_mem_virt[0] = 0x11;
        	} else if (mem_type == 2) {
				glb_mem_virt[0] = 0x12;
        	} 
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
        dma_mem_phy = 0;
	} else {
		kmalloc_mem_phy = virt_to_phys(kmalloc_mem_virt);
        pr_info("kzalloc memory at va 0x%p, pa 0x%llx, size %u \n", 
                kmalloc_mem_virt, kmalloc_mem_phy, DMA_BUFF_SZ);
	}

	glb_mem_phy = virt_to_phys(glb_mem_virt);
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

