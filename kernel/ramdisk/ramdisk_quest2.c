/*
 * Sample disk driver, from the beginning.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/version.h>

/* blk_status_t was introduced in kernel 4.12 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
typedef int blk_status_t;
#define BLK_STS_OK 0
#define BLK_STS_IOERR 10
#endif

#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT 9
#endif

/* blk_mq_init_sq_queue was introduced in kernel 4.20 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0)
#define SQ_FALLBACK
#endif

/* FIXME: implement these macros in kernel mainline */
#define size_to_sectors(size) ((size) >> SECTOR_SHIFT)
#define sectors_to_size(size) ((size) << SECTOR_SHIFT)

MODULE_LICENSE("Dual BSD/GPL");

static int sbull_major;
module_param(sbull_major, int, 0);
MODULE_PARM_DESC(sbull_major, "Major number. Default: allocate one automatically");

static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
MODULE_PARM_DESC(logical_block_size, "Logical block size. Default: 512");

static char* disk_size = "256M";
module_param(disk_size, charp, 0);
MODULE_PARM_DESC(disk_size, "Disk size. Default: 256M");

static int ndevices = 1;
module_param(ndevices, int, 0);
MODULE_PARM_DESC(ndevices, "Number of devices. Default: 1");

static bool debug = false;
module_param(debug, bool, false);
MODULE_PARM_DESC(debug, "Debug flag. Default: false");

/* the different "request modes" we can use */
enum {
	RM_DEFAULT  = 0,	/* mq op request */
	RM_BIO = 1,	/* use make_request, handle BIOs */
};

static int request_mode = RM_DEFAULT;
module_param(request_mode, int, 0);
MODULE_PARM_DESC(request_mode, "Request mode. Default: 0 (MQ request)");

/*
 * Minor number and partition management.
 */
#define SBULL_MINORS	16

/*
 * The internal representation of our device.
 */
struct sbull_dev {
	int size;                       /* Device size in sectors */
	u8 *data;                       /* The data array */
	spinlock_t lock;                /* For mutual exclusion */
	struct gendisk *gd;             /* The gendisk structure */
	struct blk_mq_tag_set tag_set;
};

static struct sbull_dev *devices;

/* Handle an I/O request */
static blk_status_t sbull_transfer(struct sbull_dev *dev, sector_t offset,
			unsigned int len, char *buffer, int op)
{
	if ((offset + len) > dev->size) {
		pr_notice("Beyond-end write (%lld %u)\n", offset, len);
		return BLK_STS_IOERR;
	}

	if  (debug)
		pr_info("%s: %s, len: %u, offset: %lld",
			dev->gd->disk_name,
			op == REQ_OP_WRITE ? "WRITE" : "READ", len,
			offset);

	/* will be only REQ_OP_READ or REQ_OP_WRITE */
	if (op == REQ_OP_WRITE)
		memcpy(dev->data + offset, buffer, len);
	else
		memcpy(buffer, dev->data + offset, len);

	return BLK_STS_OK;
}

static blk_status_t data_transfer(struct sbull_dev *dev, struct bio_vec bvec,
				sector_t *sectors, int op)
{
	blk_status_t ret;
	unsigned int len = bvec.bv_len;
	void *mem = kmap_atomic(bvec.bv_page);

	ret = sbull_transfer(dev, sectors_to_size(*sectors),
				len, mem + bvec.bv_offset, op);

	*sectors += size_to_sectors(len);
	kunmap_atomic(mem);

	return ret;
}

static blk_qc_t sbull_mq_make_request(struct request_queue *rq, struct bio *bio)
{
	struct bio_vec bvec;
	struct bvec_iter iter;
	int op = bio_op(bio);
	blk_status_t ret;
	sector_t sector = bio->bi_iter.bi_sector;
	struct sbull_dev *dev = bio->bi_disk->private_data;

	if (bio_end_sector(bio) > get_capacity(bio->bi_disk))
		goto io_error;

	if (op != REQ_OP_READ && op != REQ_OP_WRITE) {
		pr_notice("Skip non-fs request\n");
		goto io_error;
	}

	bio_for_each_segment(bvec, bio, iter) {
		ret = data_transfer(dev, bvec, &sector, op);
		/* always return BLK_QC_T_NONE for bio based request */
		if (ret != BLK_STS_OK)
			goto io_error;
	}

	bio_endio(bio);
	return BLK_QC_T_NONE;
io_error:
	bio_io_error(bio);
	return BLK_QC_T_NONE;
}

static blk_status_t sbull_queue_rq(struct blk_mq_hw_ctx *hctx,
		const struct blk_mq_queue_data *bd)
{
	struct request *req = bd->rq;
	struct sbull_dev *dev = req->rq_disk->private_data;
	int op = req_op(req);
	sector_t sector = blk_rq_pos(req);
	struct bio_vec bvec;
	struct req_iterator iter;
	blk_status_t ret = BLK_STS_OK;

	blk_mq_start_request(req);

	if (op != REQ_OP_READ && op != REQ_OP_WRITE) {
		pr_notice("Skip non-fs request\n");
		ret = BLK_STS_IOERR;
		goto io_error;
	}

	spin_lock(&dev->lock);
	rq_for_each_segment(bvec, req, iter) {
		ret = data_transfer(dev, bvec, &sector, op);
		if (ret != BLK_STS_OK)
			goto io_error;
	}
	spin_unlock(&dev->lock);
io_error:
	blk_mq_end_request(req, ret);
	return ret;
}

/*
 * The device operations structure.
 */
static const struct block_device_operations sbull_ops = {
	.owner		= THIS_MODULE,
};

static const struct blk_mq_ops sbull_mq_ops = {
	.queue_rq = sbull_queue_rq,
};

static struct request_queue *create_req_queue(struct blk_mq_tag_set *set)
{
	struct request_queue *q;

#ifndef SQ_FALLBACK
	q = blk_mq_init_sq_queue(set, &sbull_mq_ops,
			2, BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_BLOCKING);
#else
	int ret;

	memset(set, 0, sizeof(*set));
	set->ops = &sbull_mq_ops;
	set->nr_hw_queues = 1;
	/*set->nr_maps = 1;*/
	set->queue_depth = 2;
	set->numa_node = NUMA_NO_NODE;
	set->flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_BLOCKING;

	ret = blk_mq_alloc_tag_set(set);
	if (ret)
		return ERR_PTR(ret);

	q = blk_mq_init_queue(set);
	if (IS_ERR(q)) {
		blk_mq_free_tag_set(set);
		return q;
	}
#endif

	return q;
}

/*
 * Set up our internal device.
 */
static void setup_device(struct sbull_dev *dev, int which)
{
	struct request_queue *queue;
	long long sbull_size = memparse(disk_size, NULL);

	memset(dev, 0, sizeof(struct sbull_dev));
	dev->size = sbull_size;
	dev->data = vzalloc(dev->size);
	if (dev->data == NULL) {
		pr_notice("vmalloc failure.\n");
		return;
	}
	spin_lock_init(&dev->lock);

	switch (request_mode) {
	case RM_BIO:
		pr_info("Using bio request mode");
		queue = blk_alloc_queue(GFP_KERNEL);
		if (!queue)
			goto out_vfree;
		blk_queue_make_request(queue, sbull_mq_make_request);
		break;
	default:
		pr_info("Using default request mode");
		queue = create_req_queue(&dev->tag_set);
		if (IS_ERR(queue))
			goto out_vfree;
		break;
	}

	blk_queue_logical_block_size(queue, logical_block_size);
	queue->queuedata = dev;

	/* allocate the gendisk structure  */
	dev->gd = alloc_disk(SBULL_MINORS);
	if (!dev->gd) {
		pr_notice("alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = sbull_major;
	dev->gd->first_minor = which*SBULL_MINORS;
	dev->gd->fops = &sbull_ops;
	dev->gd->queue = queue;
	dev->gd->private_data = dev;
	snprintf(dev->gd->disk_name, 32, "sbull%c", which + 'a');
	set_capacity(dev->gd, size_to_sectors(sbull_size));
	add_disk(dev->gd);
	return;

out_vfree:
	if (dev->data)
		vfree(dev->data);
}

static int __init sbull_init(void)
{
	int i;

	/* registered block device */
	sbull_major = register_blkdev(sbull_major, "sbull");
	if (sbull_major <= 0) {
		pr_warn("sbull: unable to get major number\n");
		return -EBUSY;
	}

	/* allocate the device array, and initialize each one */
	devices = kmalloc(ndevices * sizeof(struct sbull_dev), GFP_KERNEL);
	if (devices == NULL)
		goto out_unregister;
	for (i = 0; i < ndevices; i++)
		setup_device(devices + i, i);

	return 0;

out_unregister:
	unregister_blkdev(sbull_major, "sbull");
	return -ENOMEM;
}

static void sbull_exit(void)
{
	int i;

	for (i = 0; i < ndevices; i++) {
		struct sbull_dev *dev = devices + i;

		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
			blk_cleanup_queue(dev->gd->queue);
		}

		if (dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(sbull_major, "sbull");
	kfree(devices);
}

module_init(sbull_init);
module_exit(sbull_exit);