#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/errno.h> 
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/cdev.h> 
#include <linux/miscdevice.h>
#include <linux/debugfs.h>
#include <linux/cpumask.h>
#include <linux/sched.h> //derek
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>

#define MISC_NAME   "kt_test"
#define CPU_CNT		8

typedef struct
{
	int enable_mask;
	int fifo_mask;
	int fifo_prio;
	int sleep_ms;
}IO_DATA_ST;

IO_DATA_ST kt_data;

struct dentry* dbg_root = NULL;

int dummy_data[CPU_CNT];
int thread_run = 1;

struct task_struct *mon_thread;
struct task_struct *kt_thread[CPU_CNT];


int kt_test_thread(void *data)
{
	int kt_index = *(int *)data;
	//struct timespec tm_begin, tm_end;

	pr_info("kt_test_thread %d start \n", kt_index);
	while (thread_run) {
		if ((kt_data.enable_mask & (1 << kt_index))) {
		} else {
			msleep(100);
		}
		
		if (kt_data.sleep_ms > 0) msleep(kt_data.sleep_ms);
		dummy_data[kt_index] += 0x10;
		//getnstimeofday(&tm_begin);
		//getnstimeofday(&tm_end);
	}

	pr_info("kt_test_thread %d exit \n", kt_index);
	return 0;
}

int kt_monitor_thread(void *data)
{
	int i;
	struct sched_param param;

	pr_info("kt_monitor_thread start \n");
	while (thread_run) {
		msleep(3000);
		pr_info("enable_mask 0x%x, fifo_mask 0x%x, fifo_prio %d, sleep_ms %d \n", 
				kt_data.enable_mask, kt_data.fifo_mask, kt_data.fifo_prio, kt_data.sleep_ms);

		for (i = 0; i < CPU_CNT; i++) {
			param.sched_priority = kt_data.fifo_prio;
			if (kt_data.fifo_mask & (1 << i)) {
				sched_setscheduler(kt_thread[i], SCHED_FIFO, &param);
			} else {
				sched_setscheduler(kt_thread[i], SCHED_NORMAL, &param);	
			}
		}
	}
	pr_info("kt_monitor_thread exit \n");
	return 0;
}

static int create_kt_task(void)
{
	int i;

	mon_thread = kthread_create(kt_monitor_thread, NULL, "kt_monitor_thread");
	if (mon_thread) {
		wake_up_process(mon_thread);
	} else {
		pr_info("create kt_monitor_thread fail\n");
		return 0;
	}
	
	for (i = 0; i < CPU_CNT; i++) {
		kt_thread[i] = kthread_create(kt_test_thread, &i, "kt_test_thread");
		if (kt_thread[i]) {
			kthread_bind(kt_thread[i], i);
			wake_up_process(kt_thread[i]);
		} else {
			pr_info("create kt_test_thread fail\n");
			return 0;
		}
		msleep(100);
	}
	
    return 0;
}

static int misc_open(struct inode *inode, struct file *file)
{
    pr_info("misc_open\n");
    return 0;
}

unsigned long dummy_cnt = 0;

static long misc_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{   
	//IO_DATA_ST temp_data;
	
	pr_info("misc_ioctl cmd 0x%x \n", cmd);
    switch(cmd)
    {
        case 0x100: //update kthread
            if(copy_from_user(&kt_data,  (int *)arg, sizeof(IO_DATA_ST)))
                return -EFAULT;
            break;
         
		default:
			break;
    }
     
    return 0;
}

static const struct file_operations misc_fops =
{
    .owner   =   THIS_MODULE,
    .open    =   misc_open,
    .unlocked_ioctl = misc_ioctl,
};

static struct miscdevice misc_dev =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = MISC_NAME,
    .fops = &misc_fops,
};
 
static int __init misc_init(void)
{
    int ret;
     
    ret = misc_register(&misc_dev);
    if (ret) {
        printk("misc_register error\n");
        return ret;
    }

    dbg_root = debugfs_create_dir("kt_test", NULL);
    if (dbg_root == NULL)
    {
        printk("debugfs_create_dir error\n");
        return ret;
    }
    debugfs_create_u32("enable_mask", 	0660, dbg_root, &kt_data.enable_mask);
    debugfs_create_u32("fifo_mask", 	0660, dbg_root, &kt_data.fifo_mask);
    debugfs_create_u32("fifo_prio", 	0660, dbg_root, &kt_data.fifo_prio);
    debugfs_create_u32("sleep_ms", 		0660, dbg_root, &kt_data.sleep_ms);

	create_kt_task();
    return 0;
}
 
static void __exit misc_exit(void)
{
	pr_info("misc_exit\n");

	thread_run = 0;
	msleep(5000);

    debugfs_remove(dbg_root);
    misc_deregister(&misc_dev);
}
 
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("derek");

