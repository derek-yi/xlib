/***************************************************************
 Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
 文件名    : ramdisk_quest.c
 作者      : 邓涛
 版本      : V1.0
 描述      : 内存模拟硬盘，实现块设备驱动，本驱动使用请求队列。
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
 * @description		: 处理传输过程
 * @param-req		: 请求
 * @return			: 无
 */
static void ramdisk_transfer(struct request *req)
{
	/* 获取写磁盘或读磁盘对应的字节地址和大小
	 * blk_rq_pos获取到的是扇区地址，左移9位转换为字节地址
	 * blk_rq_cur_bytes获取到的是大小
	 */
	unsigned long start = blk_rq_pos(req) << 9;
	unsigned long len = blk_rq_cur_bytes(req);

	/* bio中的数据缓冲区
	 * 读：从磁盘读取到的数据存放到buffer中
	 * 写：buffer保存着要写入磁盘的数据
	 */
	void *buffer = bio_data(req->bio);

	if(READ == rq_data_dir(req))	// 读数据
		memcpy(buffer, ramdisk.ramdisk_buf + start, len);
	else							// 写数据
		memcpy(ramdisk.ramdisk_buf + start, buffer, len);
}

/*
 * @description		: 请求处理函数
 * @param-q			: 请求队列
 * @return			: 无
 */
static void ramdisk_request_fn(struct request_queue *q)
{
	struct request *req;
	blk_status_t err = BLK_STS_OK;

	/* 循环处理请求队列中的每个请求 */
	req = blk_fetch_request(q);
	while(req != NULL) {

		/* 针对请求做具体的传输处理 */
		ramdisk_transfer(req);

		/* 判断当前请求是否完成，如果完成了的话获取下一个请求
		 * 循环处理完请求队列中的所有请求。
		 */
		if (!__blk_end_request_cur(req, err))
			req = blk_fetch_request(q);
	}
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

	/* 分配并初始化请求队列 */
	ramdisk.queue = blk_init_queue(ramdisk_request_fn, &ramdisk.lock);
	if(!ramdisk.queue) {
		ret = -ENOMEM;
		goto out3;
	}

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