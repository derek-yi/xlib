#include <linux/init.h>//module_init/exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE等
#include <linux/genhd.h>//alloc_disk
#include <linux/blkdev.h>//blk_init_queue
#include <linux/fs.h>//register_blkdev,unregister_blkdev
#include <linux/types.h>//u_char,u_short
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
#include <linux/bio.h>
 
#include <linux/moduleparam.h>
#include <linux/major.h>
 
 
#include <linux/highmem.h>   //kmap  kunmap
#include <linux/mutex.h>
 
#include <linux/slab.h>
 
#include <asm/uaccess.h>
#define RAMBLK_SIZE (1024*1024*2)//分配的内存2MB大小空间
 
 
/*
bio代表一个io请求，里面有io请求的所有信息
request是bio提交给io调度器产生的数据，一个request放着顺序排列的bio
request_queue代表着一个物理设备，顺序的放着request
*/
static struct gendisk * ramblk_disk = NULL;/*gendisk表示一个独立的磁盘设备，内核还可以用它来表示分区*/
static struct request_queue * ramblk_request_queue = NULL;
static int major = 0;//块设备的主设备号
static DEFINE_SPINLOCK(ramblk_spinlock);//定义并初始化一个自旋锁
static char * ramblk_buf = NULL;//申请的内存起始地址
/*
上面定义地址是用到char *，是十分有用的。类似于list中container_of一样
*/
 
int ramblk_getgeo(struct block_device * blk_Dev, struct hd_geometry * hg)
{
	printk("ramblk_getgeo\n");
	hg->cylinders = 64;
	hg->heads = 8;
	hg->sectors = (RAMBLK_SIZE/8/64/512);
	return 0;
}
 
 
/*
如果说file_operation结构是连接虚拟的VFS文件的操作与具体文件系统的文件操作之间的枢纽，那么block_device_operations就是连接抽象的块设备操作与具体块设备操作之间的枢纽。
*/
static const struct block_device_operations ramblk_fops = {
	.owner	= THIS_MODULE,
	.getgeo = ramblk_getgeo,
};
 
static void ramblk_make_request(struct request_queue *q, struct bio *bio)
{
	printk("do_ramblk_request\n");
//	struct block_device *bdev = bio->bi_bdev;
	int rw;
	struct bio_vec *bvec;
	bvec = bio->bi_io_vec;
//	sector_t sector;
	int i;
	//int err = -EIO;
	//struct request *req;
	void *disk_mem;
	void *bvec_mem;
	
	if((bio->bi_sector << 9) + bio->bi_size > RAMBLK_SIZE)
		return -EIO;
	disk_mem = ramblk_buf + (bio->bi_sector << 9);
//	sector = bio->bi_sector;
	
//	if(bio_end_sector(bio) > get_capacity(bdev->db_disk))
//		goto out;
	
	rw = bio_rw(bio);
	if(rw == READA)
		rw = READ;
	/*bio中的每个段是由一个bio_vec数据结构描述的*/
	/*
	bio_vec结构体中的字段
	struct page* bv_page 指向段的页框中页描述符的指针
	unsigned int bv_len 段的字节长度
	unsigned int bv_offset 页框中段数据的偏移量
	*/
	
	
//	 bvec_mem = kmap_atomic(bvec->bv_page) + bvec->bv_offset;
	/*高端内存映射
	允许睡眠:kmap(永久映射)
	不允许睡眠：kmap_atomic(临时映射)会覆盖以前到映射
	*/
	/*因bio_vec中的内存地址是使用page*描述的，故在高端内存中需要使用kmap进行映射才能访问，再加上
	在bio_vec中的偏移量，才是高端地址内存中的实际位置*/
	bvec_mem = kmap(bvec->bv_page) + bvec->bv_offset;  
	/*bio_for_each_segment宏定义bio.h
	#define bio_for_each_segment(bvl, bio, i) for(i=0; bvl = bio_iovec_idx((bio),(i)), i< (bio)->bi_vcnt; i++)
	bio_iovec_idx宏定义bio.h
	#define bio_iovec_idx(bio, idx) (&((bio)->bi_io_vec[(idx)]))
	*/
	bio_for_each_segment(bvec, bio, i)
	{
        /*判断bio请求处理的方向*/
        switch(rw)
        {
            case READ:
                memcpy(bvec_mem, disk_mem, bvec-> bv_len);
                break;
 
            case WRITE : 
                memcpy(disk_mem, bvec_mem, bvec-> bv_len);
                break;
            default : 
          //      kunmap_atomic(bvec->bv_page);
				kunmap(bvec->bv_page);
        }
		kunmap(bvec->bv_page);
		disk_mem += bvec->bv_len;
	}
	bio_endio(bio, 0);//bio中所有的bio_vec处理完后报告处理结束
}
 
 
static int ramblk_init(void)
{
	printk("ramblk_init\n");
	struct gendisk *disk;
//	1.分配gendisk结构体，使用alloc_disk函数
/*
	gendisk结构是一个动态分配的结构， 它需要一些内核的特殊处理来进行初始化，驱动程序不能自己动态分配该架构
	而使用struct gendisk *alloc_disk(int mimors) 参数minors是该磁盘使用的次设备号的数目
*/
	
//	2.设置
//	2.1 分配/设置队列，提供读写能力.使用函数blk_init_queue(request_fn_proc *rfn,spin_lock_t *lock)
//	ramblk_request_queue = blk_init_queue(ramblk_make_request,&ramblk_spinlock);
 
	major = register_blkdev(0,"sbull");//注册主设备
	if(major < 0){//检查是否成功分配一个有效的主设备号
		printk(KERN_ALERT "register_blkdev error.\n");
		return -1;
	}
	/*使用制造请求的方式，先分配queue*/
	ramblk_request_queue = blk_alloc_queue(GFP_KERNEL);
	/*在绑定请求制造函数*/
	blk_queue_make_request(ramblk_request_queue, ramblk_make_request);
	disk = ramblk_disk = alloc_disk(16);//minors=分区+1 
	
//	2.2 设置disk的其他信息，比如容量、主设备号等
	
	
	//设置主设备号
	ramblk_disk->major = major;
	ramblk_disk->first_minor = 0;//设置第一个次设备号
	ramblk_disk->minors=1;//设置最大的次设备号，=1表示磁盘不能被分区
	sprintf(ramblk_disk->disk_name, "sbull%c", 'a');//设置设备名
	ramblk_disk->fops = &ramblk_fops;//设置fops  设置前面表述的各种设备操作
	ramblk_disk->queue = ramblk_request_queue;//设置请求队列
	set_capacity(ramblk_disk, RAMBLK_SIZE/512);//设置容量
	
//	3.硬件相关的操作
	ramblk_buf = (char*)vmalloc(RAMBLK_SIZE);//申请RAMBLK_SIZE内存
	
//	4.注册
	add_disk(ramblk_disk);//add partitioning information to kernel list
	printk("ramblk_init.\n");
	return 0;
}
 
static void ramblk_exit(void)
{
	del_gendisk(ramblk_disk);
	put_disk(ramblk_disk);
	unregister_blkdev(major,"sbull");//注销设备驱动
	blk_cleanup_queue(ramblk_request_queue);//清除队列
	
	vfree(ramblk_buf);//释放申请的内存
	printk("ramblk_exit.\n");
}
 
 
module_init(ramblk_init);//入口
module_exit(ramblk_exit);//出口
 
MODULE_AUTHOR("hustcs");
MODULE_LICENSE("Dual BSD/GPL");
