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

#define MISC_NAME   "misc_op_cache"

typedef struct
{
	void *buff;
	unsigned long len;
}IO_DATA_ST;

static int misc_open(struct inode *inode, struct file *file)
{
    pr_info("misc_open\n");
    return 0;
}

static inline void __flush_dcache_all(void)
{
	__asm__ __volatile__("dmb sy\n\t"
			"mrs x0, clidr_el1\n\t"
			"and x3, x0, #0x7000000\n\t"
			"lsr x3, x3, #23\n\t"
			"cbz x3, 5f\n\t"
			"mov x10, #0\n\t"
		"1:\n\t"
			"add x2, x10, x10, lsr #1\n\t"
			"lsr x1, x0, x2\n\t"
			"and x1, x1, #7\n\t"
			"cmp x1, #2\n\t"
			"b.lt    4f\n\t"
			"mrs x9, daif\n\t"
			"msr daifset, #2\n\t"
			"msr csselr_el1, x10\n\t"
			"isb\n\t"
			"mrs x1, ccsidr_el1\n\t"
			"msr daif, x9\n\t"
			"and x2, x1, #7\n\t"
			"add x2, x2, #4\n\t"
			"mov x4, #0x3ff\n\t"
			"and x4, x4, x1, lsr #3\n\t"
			"clz w5, w4\n\t"
			"mov x7, #0x7fff\n\t"
			"and x7,x7, x1, lsr #13\n\t"
		"2:\n\t"
			"mov x9, x4\n\t"
		"3:\n\t"
			"lsl x6, x9, x5\n\t"
			"orr x11, x10, x6\n\t"
			"lsl x6, x7, x2\n\t"
			"orr x11, x11, x6\n\t"
			"dc  cisw, x11\n\t"
			"subs    x9, x9, #1\n\t"
			"b.ge    3b\n\t"
			"subs    x7, x7, #1\n\t"
			"b.ge    2b\n\t"
		"4:\n\t"
			"add x10, x10, #2\n\t"
			"cmp x3, x10\n\t"
			"b.gt    1b\n\t"
		"5:\n\t"
			"mov x10, #0\n\t"
			"msr csselr_el1, x10\n\t"
			"dsb sy\n\t"
			"isb\n\t");

}

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

int flush_user_pa(IO_DATA_ST *iocb)
{
	void *virt_addr = phys_to_virt((unsigned long)iocb->buff);
	__flush_dcache_area(virt_addr, iocb->len);
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

int invalid_user_pa(IO_DATA_ST *iocb)
{
	void *virt_addr = phys_to_virt((unsigned long)iocb->buff);
	__inval_dcache_area(virt_addr, iocb->len);
	return 0;
}

unsigned long dummy_cnt = 0;

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	IO_DATA_ST temp_data;
	
	pr_info("misc_ioctl cmd 0x%x \n", cmd);
    switch(cmd)
    {
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
         
        case 0x101: 
			__flush_dcache_all();
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

		case 0x300: 
			dummy_cnt++;
			break;
			
		default:
			break;
    }
     
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
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }
 
    return 0;
}
 
static void __exit misc_exit(void)
{
	pr_info("misc_exit\n");
    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("derek");

