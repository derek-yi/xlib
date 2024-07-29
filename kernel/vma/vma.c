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

#define DEVICE_NAME "vma_demo"

#define DMA_BUFF_SZ 4096

static unsigned char *share_mem;

char glb_mem_virt[DMA_BUFF_SZ];

static int pid;

static void print_task_info(struct task_struct *tsk)
{
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    int j = 0;
    unsigned long start, end, length;

    mm = tsk->mm;
    pr_info("mm_struct addr = 0x%p\n", mm);
    vma = mm->mmap;

    /* 使用 mmap_sem 读写信号量进行保护 */
    //down_read(&mm->mmap_sem);

    while (vma) {
        j++;
        start = vma->vm_start;
        end = vma->vm_end;
        length = end - start;
        pr_info("%6d: %16p %12lx %12lx %8ld flag 0x%lx\n",
                j, vma, start, end, length, vma->vm_flags);
        vma = vma->vm_next;
    }
    //up_read(&mm->mmap_sem);
}

static int my_open(struct inode *inode, struct file *file)
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
 
static int my_map(struct file *filp, struct vm_area_struct *vma)
{
    vma->vm_flags |= VM_IO; //keep coherent
    //vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

#if 1
    if (remap_pfn_range(vma,
                       vma->vm_start,
                       virt_to_phys(share_mem)>>PAGE_SHIFT,
                       vma->vm_end - vma->vm_start,
                       vma->vm_page_prot))
    {  
        return -EAGAIN;  
    }
#else
    if (remap_pfn_range(vma,
                       vma->vm_start,
                       (virt_to_phys(glb_mem_virt)>>PAGE_SHIFT) + vma->vm_pgoff,
                       vma->vm_end - vma->vm_start,
                       vma->vm_page_prot))
    {  
        return -EAGAIN;  
    }
#endif

    return 0;
}
 
static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .mmap = my_map,
};
 
static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,
};
 
static ssize_t ext_sync_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "pid %d \n", pid);
}

static ssize_t ext_sync_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
    struct task_struct *tsk;

	if ( memcmp(buf, "show", 4) == 0 ) {
        pid = current->pid;
        pr_info("current pid %d\n", pid);
        pr_info("share_mem 0x%lx, phy 0x%lx \n", (long)share_mem, (long)virt_to_phys(share_mem));
	}
	else {
        pid = current->pid;
        pr_info("current pid %d\n", pid);
		pid = simple_strtol(buf, 0, 0);
        if (pid > 0) {
            tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
            if (tsk) print_task_info(tsk);
        }
	}

	return len;
}

static DEVICE_ATTR(cmd_buffer, 0660, ext_sync_show, ext_sync_store);
 
static int __init dev_init(void)
{
    int ret;
    unsigned char i;
    struct device *dev;

    //内存分配
    share_mem = (unsigned char *)kmalloc(DMA_BUFF_SZ, GFP_KERNEL);
    //share_mem = (unsigned char *)vmalloc(DMA_BUFF_SZ);
    for (i = 0; i < 128; i++) share_mem[i] = i;
    printk("share_mem 0x%lx virt_to_phys 0x%llx \n", (long)share_mem, virt_to_phys(share_mem));

    for (i = 0; i < 128; i++) glb_mem_virt[i] = i;
    printk("glb_mem_virt 0x%lx virt_to_phys 0x%llx \n", (long)glb_mem_virt, virt_to_phys(glb_mem_virt));

    //将该段内存设置为保留
    //SetPageReserved(virt_to_page(share_mem));

    //注册混杂设备
    ret = misc_register(&misc);

    dev = misc.this_device;
    if (sysfs_create_file(&dev->kobj, &dev_attr_cmd_buffer.attr)) {
        dev_info(dev, "%s: sysfs create failed", __func__);
        return -1;
    }

    return ret;
}
 
static void __exit dev_exit(void)
{
    //注销设备
    misc_deregister(&misc);

    //清除保留
    //ClearPageReserved(virt_to_page(share_mem));

    //释放内存
    kfree(share_mem);
    //vfree(share_mem);
}
 
module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");




