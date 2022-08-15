/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : ramdisk_noquest.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 内存模拟硬盘，实现块设备驱动，本驱动不使用请求队列。
 其他      : 无
 论坛      : www.openedv.com
 日志      : 初版V1.0 2020/7/26 邓涛创建
 ***************************************************************/

#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>


#define RAMDISK_SIZE	(2 * 1024 * 1024U)		/* 容量大小为2MB */
#define RAMDISK_NAME	"ramdisk"				/* 名字 */
#define RADMISK_MINOR	3		/* 表示三个磁盘分区！不是次设备号为3！ */

/* ramdisk设备结构体 */
struct ramdisk_dev {
	int major;					/* 主设备号 */
	unsigned char *ramdisk_buf;	/* ramdisk内存空间,用于模拟块设备 */
	spinlock_t lock;			/* 自旋锁 */
	struct gendisk *gendisk;	/* gendisk */
	struct request_queue *queue;/* 请求队列 */
};

static struct ramdisk_dev ramdisk;	/* ramdisk设备 */

/*
 * @description		: "制造请求"函数
 * @param-q			: 请求队列
 * @return			: 无
 */
static blk_qc_t ramdisk_make_request_fn(struct request_queue *q,
			struct bio *bio)
{
	int offset;
	struct bio_vec bvec;
	struct bvec_iter iter;
	unsigned long len = 0;

	offset = (bio->bi_iter.bi_sector) << 9; /* 获取设备的偏移地址 */

	/* 处理bio中的每个段 */
	bio_for_each_segment(bvec, bio, iter){
		char *ptr = page_address(bvec.bv_page) + bvec.bv_offset;
		len = bvec.bv_len;

		if(bio_data_dir(bio) == READ)	// 读数据
			memcpy(ptr, ramdisk.ramdisk_buf + offset, len);
		else							// 写数据
			memcpy(ramdisk.ramdisk_buf + offset, ptr, len);
		offset += len;
	}

	bio_endio(bio);
	return BLK_QC_T_NONE;
}

/*
 * @description		: 打开块设备
 * @param - dev		: 块设备
 * @param - mode	: 打开模式
 * @return			: 0 成功;其他 失败
 */
static int ramdisk_open(struct block_device *dev, fmode_t mode)
{
	printk(KERN_INFO "ramdisk open\n");
	return 0;
}

/*
 * @description		: 释放块设备
 * @param - disk	: gendisk
 * @param - mode	: 模式
 * @return			: 0 成功;其他 失败
 */
static void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
	printk("ramdisk release\r\n");
}

/*
 * @description		: 获取磁盘信息
 * @param - dev		: 块设备
 * @param - geo		: 模式
 * @return			: 0 成功;其他 失败
 */
static int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
	/* 这是相对于机械硬盘的概念 */
	geo->heads = 2;								// 磁头
	geo->cylinders = 32;						// 柱面
	geo->sectors = RAMDISK_SIZE / (2 * 32 *512);// 磁道上的扇区数量
	return 0;
}

static struct block_device_operations ramdisk_fops = {
	.owner		= THIS_MODULE,
	.open		= ramdisk_open,
	.release	= ramdisk_release,
	.getgeo		= ramdisk_getgeo,
};

static int __init ramdisk_init(void)
{
	int ret;

	/* 为ramdisk分配内存 */
	ramdisk.ramdisk_buf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
	if(NULL == ramdisk.ramdisk_buf)
		return -ENOMEM;

	/* 初始化自旋锁 */
	spin_lock_init(&ramdisk.lock);

	/* 注册块设备 */
	ramdisk.major = register_blkdev(0, RAMDISK_NAME);
	if(ramdisk.major < 0) {
		ret = ramdisk.major;
		goto out1;
	}

	printk(KERN_INFO "ramdisk major=%d\n", ramdisk.major);

	/* 分配并初始化gendisk */
	ramdisk.gendisk = alloc_disk(RADMISK_MINOR);
	if(!ramdisk.gendisk) {
		ret = -ENOMEM;
		goto out2;
	}

	/* 分配请求队列 */
	ramdisk.queue = blk_alloc_queue(GFP_KERNEL);
	if(!ramdisk.queue){
		ret = -ENOMEM;
		goto out3;
	}

	/* 设置"制造请求"函数 */
	blk_queue_make_request(ramdisk.queue, ramdisk_make_request_fn);

	/* 添加(注册)disk */
	ramdisk.gendisk->major = ramdisk.major;				// 主设备号
	ramdisk.gendisk->first_minor = 0;					// 起始次设备号
	ramdisk.gendisk->fops = &ramdisk_fops;				// 操作函数
	ramdisk.gendisk->private_data = &ramdisk;			// 私有数据
	ramdisk.gendisk->queue = ramdisk.queue;				// 请求队列
	strcpy(ramdisk.gendisk->disk_name, RAMDISK_NAME);	// 名字
	set_capacity(ramdisk.gendisk, RAMDISK_SIZE / 512);	// 设备容量(单位为扇区)

	add_disk(ramdisk.gendisk);
	return 0;

out3:
	put_disk(ramdisk.gendisk);

out2:
	unregister_blkdev(ramdisk.major, RAMDISK_NAME);

out1:
	kfree(ramdisk.ramdisk_buf);
	return ret;
}

static void __exit ramdisk_exit(void)
{
	/* 卸载gendisk */
	del_gendisk(ramdisk.gendisk);

	/* 清除请求队列 */
	blk_cleanup_queue(ramdisk.queue);

	/* 释放gendisk */
	put_disk(ramdisk.gendisk);

	/* 注销块设备 */
	unregister_blkdev(ramdisk.major, RAMDISK_NAME);

	/* 释放内存 */
	kfree(ramdisk.ramdisk_buf);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_AUTHOR("Deng Tao <773904075@qq.com>, ALIENTEK, Inc.");
MODULE_DESCRIPTION("Memory Simulation Block Device Driver");
MODULE_LICENSE("GPL");

/*
sudo mkfs.ext4 /dev/ramdisk
sudo mount -t ext4 /dev/ramdisk ./tmp
sudo umount ./tmp

*/
