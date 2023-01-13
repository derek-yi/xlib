#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/gpio.h>
 
#define DEVICE_NAME "mymap"
 
unsigned char *buffer;

static int my_open(struct inode *inode, struct file *file)
{
    return 0;
}
 
static int my_map(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long phys;

    //得到物理地址
    phys = virt_to_phys(buffer);
    
    //将用户空间的一个vma虚拟内存区映射到以page开始的一段连续物理页面上
    if(remap_pfn_range(vma,
                    vma->vm_start,
                    phys >> PAGE_SHIFT,//第三个参数是页帧号，由物理地址右移PAGE_SHIFT得>到
                    vma->vm_end - vma->vm_start,
                    vma->vm_page_prot))
            return -1;

    return 0;
}

typedef struct
{
	unsigned long addr;
	unsigned long len;
	int ret;
	int param[7];
}IO_DATA_ST;

int flush_user_va1(IO_DATA_ST *io_data)
{
	unsigned long len = io_data->len;
	char *buf = io_data->addr;
	struct page *pages[64];
	unsigned int pg_off = offset_in_page(buf);
	unsigned int pages_nr = (len + pg_off + PAGE_SIZE - 1) >> PAGE_SHIFT;
	int i;
	int rv;

	if ( (pages_nr == 0) || (pages_nr > 64) ) {
		pr_err("pages_nr %u", pages_nr);
		return -EINVAL;
	}

	memset(pages, 0, pages_nr * sizeof(struct page *));
	//rv = get_user_pages_fast((unsigned long)buf, pages_nr, 1/* write */, pages);
	rv = get_user_pages_fast((unsigned long)buf, pages_nr, 0, pages);
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
		phys_addr_t phy_addr = virt_to_phys(page_address(pg));
		pr_info("user_addr 0x%lx, phy_addr 0x%llx, virt_addr 0x%p \n", 
                io_data->addr, phy_addr, page_address(pg));
		//__flush_dcache_area(page_address(pg), PAGE_SIZE);
	}

err_out:
	//kfree(pages);
	for (i = 0; i < pages_nr; i++) {
		if(pages[i]) put_page(pages[i]);
	}

	return 0;
}

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
    IO_DATA_ST io_data;
    
    printk(KERN_NOTICE"ioctl CMD %d \n", cmd);   
    switch (cmd)
    {
        case 0x100:
            if (copy_from_user(&io_data,  (int *)arg, sizeof(IO_DATA_ST)))
                return -EFAULT;

            flush_user_va1(&io_data);
            io_data.ret = 999;
            if(copy_to_user( (int *)arg, &io_data, sizeof(IO_DATA_ST)))
                return -EFAULT;
            break;
         
        default:
            break;
    }
     
    return 0;
}

static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .mmap = my_map,
    .unlocked_ioctl = misc_ioctl,
};
 
static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,
};

static ssize_t hwrng_attr_current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int i, j;

    for (i = 0, j = 0; i < 16; i++) {
        j += sprintf(buf + j, "%d ", buffer[i]);
    }

    j += sprintf(buf + j, "\n");
    return j;
}
                    
static DEVICE_ATTR(rng_current, S_IRUGO, hwrng_attr_current_show, NULL);
 
static int __init dev_init(void)
{
    int ret;
    unsigned char i;

    //内存分配
    buffer = (unsigned char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (buffer == NULL) return -1;

    printk("virt_addr 0x%lx, phy_addr 0x%lx, page %d \n", 
            buffer, virt_to_phys(buffer), virt_to_page(buffer));

    //driver起来时初始化内存前10个字节数据
    for(i = 0; i < 32; i++) buffer[i] = i;
    
    //将该段内存设置为保留
    SetPageReserved(virt_to_page(buffer));

    //注册混杂设备
    ret = misc_register(&misc);
    ret = device_create_file(misc.this_device, &dev_attr_rng_current);

    return ret;
}

static void __exit dev_exit(void)
{
    device_remove_file(misc.this_device, &dev_attr_rng_current);
    
    //注销设备
    misc_deregister(&misc);
    
    //清除保留
    ClearPageReserved(virt_to_page(buffer));
    
    //释放内存
    kfree(buffer);
}
 
module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKN@SCUT");

