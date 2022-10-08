#include <linux/init.h>  
#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/types.h>  
#include <linux/fcntl.h>  
#include <linux/vmalloc.h>  
#include <linux/hdreg.h>  
#include <linux/blkdev.h>  
#include <linux/blkpg.h>  
#include <asm/uaccess.h>  
  
#define BLK_NAME "ram_blk"  
#define BLK_MAJOR 222  
#define DISK_SECTOR_SIZE 512 //每扇区大小  
#define DISK_SECTOR 1024  //总扇区数，  
#define DISK_SIZE (DISK_SECTOR_SIZE*DISK_SECTOR)//总大小，共0.5M  
  
typedef struct//设备结构体  
{  
       unsigned char          *data;  
       struct request_queue   *queue;  
       struct gendisk         *gd;  
} disk_dev;  
  
disk_dev device;//定义设备结构体  
  
//--------------------------------------------------------------------------  
//在硬盘等带柱面扇区等的设备上使用request，可以整理队列。但是ramdisk等可以  
//使用make_request  
static int disk_make_request(struct request_queue *q,struct bio *bio)  
{  
       int i;  
       char *mem_pbuf;  
       char *disk_pbuf;  
       disk_dev *pdevice;  
       struct bio_vec *pbvec;  
       /*在遍历段之前先判断要传输数据的总长度大小是否超过范围*/  
       i=bio->bi_sector*DISK_SECTOR_SIZE+bio->bi_size;  
       if(i>DISK_SIZE)//判断是否超出范围  
              goto fail;  
         
       pdevice=(disk_dev*)bio->bi_bdev->bd_disk->private_data;//得到设备结构体  
       disk_pbuf=pdevice->data+bio->bi_sector*DISK_SECTOR_SIZE;//得到要读写的起始位置  
         
       /*开始遍历这个bio中的每个bio_vec*/  
       bio_for_each_segment(pbvec,bio,i)//循环分散的内存segment  
       {  
              mem_pbuf=kmap(pbvec->bv_page)+pbvec->bv_offset;//获得实际内存地址  
              switch(bio_data_dir(bio))  
              {//读写  
                     case READA:  
                     case READ:  
                            memcpy(mem_pbuf,disk_pbuf,pbvec->bv_len);  
                            break;  
                     case WRITE:  
                            memcpy(disk_pbuf,mem_pbuf,pbvec->bv_len);  
                            break;  
                     default:  
                            kunmap(pbvec->bv_page);  
                            goto fail;  
              }  
              kunmap(pbvec->bv_page);//清除映射  
              disk_pbuf+=pbvec->bv_len;  
       }  
       bio_endio(bio,0);//这个函数2.6.25和2.6.4是不一样的，  
       return 0;  
fail:  
       bio_io_error(bio);//这个函数2.6.25和2.6.4是不一样的，  
       return 0;  
}  
  
int blk_open(struct block_device *dev, fmode_t no)   
{  
       return 0;  
}  
  
int blk_release(struct gendisk *gd, fmode_t no)  
{  
       return 0;  
}  
  
int blk_ioctl(struct block_device *dev, fmode_t no, unsigned cmd, unsigned long arg)  
{  
       return -ENOTTY;  
}  
  
static struct block_device_operations blk_fops=  
{  
       .owner=THIS_MODULE,  
       .open=blk_open,//  
       .release=blk_release,//  
       .ioctl=blk_ioctl,//   
};  
  
int disk_init(void)  
{  
        if(!register_blkdev(BLK_MAJOR,BLK_NAME));//注册驱动  
    {  
         printk("register blk_dev succeed\n");  
    }  
      
       device.data=vmalloc(DISK_SIZE);  
       device.queue=blk_alloc_queue(GFP_KERNEL);//生成队列  
       blk_queue_make_request(device.queue,disk_make_request);/*注册make_request  绑定请求制造函数*/  
  
    printk("make_request succeed\n");  
  
       device.gd=alloc_disk(1);//生成gendisk  
       device.gd->major=BLK_MAJOR;//主设备号  
       device.gd->first_minor=0;//此设备号  
       device.gd->fops=&blk_fops;//块文件结构体变量  
       device.gd->queue=device.queue;//请求队列  
       device.gd->private_data=&device;  
       sprintf(device.gd->disk_name,"disk%c",'a');//名字  
       set_capacity(device.gd,DISK_SECTOR);//设置大小  
       add_disk(device.gd);//注册块设备信息  
    printk("gendisk succeed\n");      
       return 0;  
}  
  
void disk_exit(void)  
{  
      
       del_gendisk(device.gd);  
       put_disk(device.gd);  
       unregister_blkdev(BLK_MAJOR,BLK_NAME);  
       vfree(device.data);  
        printk("free succeed\n");  
      
}  
  
module_init(disk_init);  
module_exit(disk_exit);  
  
MODULE_LICENSE("Dual BSD/GPL");  
