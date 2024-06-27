
// 内核模块
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("godjesse");
MODULE_DESCRIPTION("mmap demo");

static unsigned long p = 0;
static unsigned long pp = 0;

static int __init init(void)
{
    p = __get_free_pages(GFP_KERNEL, 0);

    if(!p)
    {
        printk("Allocate memory failure!/n");
    }
    else
    {
        SetPageReserved(virt_to_page(p));
        // 使用virt_to_phys计算物理地址，供用户态程序使用
        pp = (unsigned long)virt_to_phys((void *)p);
        printk("<1> page : pp = 0x%lx\n",pp);
    }
    strcpy((char *)p, "Hello world !\n");
    return 0;
}

static void __exit fini(void)
{
    printk("The content written by user is: %s/n", (unsigned char *)p);
    ClearPageReserved(virt_to_page(p));
    free_pages(p, 0);
    printk(" exit \n");
}

module_init(init);
module_exit(fini);

//---------------------------------------------------------------------------------------------------------------------------------------

// 用户态程序
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

//hard coding read after the module installed
#define KERNEL_PHY_ADDR  0x3737c000

int main()
{
    char *buf;
    int fd;
    unsigned long phy_addr;
    int  pagesize = getpagesize();

    phy_addr=KERNEL_PHY_ADDR;

    fd = open("/dev/mem",O_RDWR);
    if(fd == -1)
        perror("open");

    buf = mmap(0, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phy_addr);
    if(buf == MAP_FAILED)
    {
        perror("mmap");
    }

    printf("buf : %s\n",buf);

    // test the write 
    buf[0] = 'X';

    munmap(buf,pagesize);
    close(fd);
    return 0;

}

