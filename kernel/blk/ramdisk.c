
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#define RAMHD_NAME              "ramhd"
#define RAMHD_MAX_DEVICE        2
#define RAMHD_MAX_PARTITIONS    4

#define RAMHD_SECTOR_SIZE       512
#define RAMHD_SECTORS           16
#define RAMHD_HEADS             4
#define RAMHD_CYLINDERS         256

#define RAMHD_SECTOR_TOTAL      (RAMHD_SECTORS * RAMHD_HEADS *RAMHD_CYLINDERS)
#define RAMHD_SIZE              (RAMHD_SECTOR_SIZE * RAMHD_SECTOR_TOTAL) //8mb

typedef struct {
    unsigned char* data;
    struct request_queue* queue;
    struct gendisk* gd;
}RAMHD_DEV;

static char* sdisk[RAMHD_MAX_DEVICE] = {NULL};
static RAMHD_DEV* rdev[RAMHD_MAX_DEVICE] = {NULL};

static dev_t ramhd_major;

static int ramhd_space_init(void)
{
    int i;
    int err = 0;
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        sdisk[i] = vmalloc(RAMHD_SIZE);
        if(!sdisk[i]){
            err = -ENOMEM;
            return err;
        }
        
        memset(sdisk[i], 0, RAMHD_SIZE);
    }
    
    return err;
}

static void ramhd_space_clean(void)
{
    int i;
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        vfree(sdisk[i]);
    }
}

static int ramhd_open(struct block_device* bdev, fmode_t mode)
{
    return 0;
}

static void ramhd_release(struct gendisk*gd, fmode_t mode)
{
    return 0;
}

static int ramhd_ioctl(struct block_device* bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
    int err;
    struct hd_geometry geo;
    
    switch(cmd)
    {
        case HDIO_GETGEO:
            err = !access_ok(VERIFY_WRITE, arg, sizeof(geo));
            if(err)
                return -EFAULT;
                
            geo.cylinders = RAMHD_CYLINDERS;
            geo.heads = RAMHD_HEADS;
            geo.sectors = RAMHD_SECTORS;
            geo.start = get_start_sect(bdev);
            
            if(copy_to_user((void*)arg, &geo, sizeof(geo)))
                return -EFAULT;
            
            return 0;
    }
    
    return -ENOTTY;
}

static struct block_device_operations ramhd_fops = {
    .owner = THIS_MODULE,
    .open = ramhd_open,
    .release = ramhd_release,
    .ioctl = ramhd_ioctl,
};

static blk_qc_t ramhd_make_request(struct request_queue* q, struct bio* bio)
{
    char* pRHdata;
    char* pBuffer;
    struct bio_vec bvec;
	struct bvec_iter iter;
    int err = 0;
    
    //struct block_device* bdev = bio->bi_bdev;
    RAMHD_DEV* pdev = bio->bi_disk->private_data;
    
    if (((bio->bi_iter.bi_sector * RAMHD_SECTOR_SIZE) + bio->bi_iter.bi_size) > RAMHD_SIZE){
        err = -EIO;
        return err;
    }
    
    pRHdata = pdev->data + (bio->bi_iter.bi_sector * RAMHD_SECTOR_SIZE);
    bio_for_each_segment(bvec, bio, iter) {
        pBuffer = kmap(bvec.bv_page) + bvec.bv_offset;
        switch(bio_data_dir(bio)){
            case READ:
                memcpy(pBuffer, pRHdata, bvec.bv_len);
                flush_dcache_page(bvec.bv_page);
                break;
                
            case WRITE:
                flush_dcache_page(bvec.bv_page);
                memcpy(pRHdata, pBuffer, bvec.bv_len);
                break;
                
            default:
                kunmap(bvec.bv_page);
                goto out;
        }
        
        kunmap(bvec.bv_page);
        pRHdata += bvec.bv_len;
    }
    
out:
    bio_endio(bio);
    return 0;
}

static int alloc_ramdev(void)
{
    int i;
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        rdev[i] = kzalloc(sizeof(RAMHD_DEV), GFP_KERNEL);
        if(!rdev[i]){
            return -ENOMEM;
        }
    }
    
    return 0;
}

static void clean_ramdev(void)
{
    int i;
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        if(rdev[i])
            kfree(rdev[i]);
    }
}

static int __init ramhd_init(void)
{
    int i;
    
    ramhd_space_init();
    alloc_ramdev();
    
    ramhd_major = register_blkdev(0, RAMHD_NAME);
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        rdev[i]->data = sdisk[i];
        rdev[i]->queue = blk_alloc_queue(GFP_KERNEL);
        blk_queue_make_request(rdev[i]->queue, ramhd_make_request);
        
        rdev[i]->gd = alloc_disk(RAMHD_MAX_PARTITIONS);
        rdev[i]->gd->major = ramhd_major;
        rdev[i]->gd->first_minor = i * RAMHD_MAX_PARTITIONS;
        rdev[i]->gd->fops = &ramhd_fops;
        rdev[i]->gd->queue = rdev[i]->queue;
        rdev[i]->gd->private_data = rdev[i];
        sprintf(rdev[i]->gd->disk_name, "ramhd%c", 'a' +i);
        rdev[i]->gd->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
        set_capacity(rdev[i]->gd, RAMHD_SECTOR_TOTAL);
        add_disk(rdev[i]->gd);
    }
    
    return 0;
}

static void __exit ramhd_exit(void)
{
    int i;
    
    for(i = 0; i < RAMHD_MAX_DEVICE; i++){
        del_gendisk(rdev[i]->gd);
        put_disk(rdev[i]->gd);
        blk_cleanup_queue(rdev[i]->queue);
    }
    
    clean_ramdev();
    ramhd_space_clean();
    unregister_blkdev(ramhd_major, RAMHD_NAME);
}

module_init(ramhd_init);
module_exit(ramhd_exit);

MODULE_AUTHOR("dennis__chen@ AMDLinuxFGL");
MODULE_DESCRIPTION("The ramdisk implementation with request function");
MODULE_LICENSE("GPL");


/*
  a）make 一下，生成ramdisk.ko；

  b）编译好了之后，就可以安装驱动了，在linux下是这么做的，sudo insmod ramdisk.ko；

  c）安装好了，利用ls /dev/ramhd*， 就会发现在/dev下新增两个结点，即/dev/ramhda和/dev/ramhdb；

  d）不妨选择其中一个节点进行分区处理， sudo fdisk /dev/ramhda，简单处理的话就建立一个分区， 生成/dev/ramhda1；

  e）创建文件系统，sudo mkfs.ext3 /dev/ramhda1；

  f）有了上面的文件系统，就可以进行mount处理，不妨sudo mount /dev/ramhda1 /mnt；

  g）上面都弄好了，大家就可以copy、delete文件试试了，是不是很简单。

*/

