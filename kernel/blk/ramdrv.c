
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/genhd.h>
#include<linux/blkdev.h>
#include<linux/bio.h>
 
 
#define DEVICE_MAJOR    240
#define DEVICE_NAME     "ramdisk"
 
 
#define SECTOR_SIZE 512
//扇区大小

#define DISK_SIZE (3*1024*1024)
//虚拟磁盘大小

#define SECTOR_ALL (DISK_SIZE/SECTOR_SIZE)
//虚拟磁盘的扇区数
 
 
static struct gendisk *p_disk; //用来指向申请的gendisk结构体
static struct request_queue *p_queue; //用来指向请求队列
static unsigned char mem_start[DISK_SIZE]; //分配一块3M 的内存作为虚拟磁盘
 

/***********************************************************************
对于ram、loop、网络设备等使用自己编写的make_request函数来处理bio
省去使用内核I/O调度器的过程，make_request返回值是0
**********************************************************************/
static void ramdisk_make_request(struct request_queue *q,struct bio *bio)
{
    struct bio_vec *bvec; //bio结构中包含多个bio_vec结构
    int i; //用于循环的变量，不需要赋值
    void *disk_mem; //指向虚拟磁盘正在读写的位置
    
    if((bio->bi_sector*SECTOR_SIZE)+bio->bi_size>DISK_SIZE)
    { //检查超出容量的情况
        printk("ramdisk over flowed!\n");
        bio_endio(bio,1); //第二个参数为1通知内核bio处理出错
        return ;
    }

    disk_mem=mem_start+bio->bi_sector*SECTOR_SIZE; //mem_start是虚拟磁盘的起始地址

    //bio_for_each_segment是一个for循环的宏，每次循环处理一个bio_vec
    bio_for_each_segment(bvec,bio,i){
        void *iovec; //指向内核存放数据的地址
        iovec=kmap(bvec->bv_page)+bvec->bv_offset; //将bv_page映射到高端内存
        switch(bio_data_dir(bio)){ //bio_data_dir(bio)返回要处理数据的方向
            case READA: //READA是预读，RAED是读，采用同一处理
            case READ:memcpy(iovec,disk_mem,bvec->bv_len);break;
            case WRITE:memcpy(disk_mem,iovec,bvec->bv_len);break;
            default:bio_endio(bio,1);kunmap(bvec->bv_page);return ; //处理失败的情况
        }
        kunmap(bvec->bv_page); //释放bv_page的映射
        disk_mem+=bvec->bv_len; //移动虚拟磁盘的指向位置，准备下一个循环bvec的读写做准备
    }
    
    bio_endio(bio,0); //第二个参数为0通知内核处理成功
    return ;
}


static struct block_device_operations ramdisk_fops={
    .owner=THIS_MODULE,
};


static int ramdisk_init(void)
{
    p_queue=blk_alloc_queue(GFP_KERNEL); //申请请求队列，不附加make_request函数
    if(!p_queue)return -1;
    blk_queue_make_request(p_queue, ramdisk_make_request); //将自己编写的make_request函数添加到申请的队列
    p_disk=alloc_disk(1); //申请一个分区的gendisk结构体
    if(!p_disk)
    {
        blk_cleanup_queue(p_queue); //gendisk申请失败，清除已申请的请求队列
        return -1;
    }
    strcpy(p_disk->disk_name,DEVICE_NAME); //块设备名
    p_disk->major=DEVICE_MAJOR; //主设备号
    p_disk->first_minor=0; //次设备号
    p_disk->fops=&ramdisk_fops; //fops地址
    p_disk->queue=p_queue; //请求队列地址
    set_capacity(p_disk,SECTOR_ALL); //设置磁盘扇区数
    add_disk(p_disk); //设置好后添加这个磁盘
    return 0;
}


static void ramdisk_exit(void)
{
    del_gendisk(p_disk); //删除gendisk注册信息
    put_disk(p_disk); //释放disk空间
    blk_cleanup_queue(p_queue); //清除请求队列
}
 
 
module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");

/*

测试步骤：
lsmodinsmod ramdrv2.ko //动态挂载驱动模块 （文件名称）
lsmod //查看是否多了个ramdrv2模块（文件名称）
ls /dev //查看一下/dev目录下是否多了个ramdisk节点（次名称代码中定义）
mkfs.ext3 /dev/ramdisk //在虚拟磁盘上建立ext3文件系统
mount /dev/ramdisk /mnt/test //挂载到/mnt/test目录，是不是多了lost+found文件夹？
mount //查看mount的记录

卸载：umount /mnt/testrmmod ramdisk_driver

作者：力卉编程链接：https://www.jianshu.com/p/753e70061037

*/

