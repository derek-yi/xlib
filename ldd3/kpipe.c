/****************************************************************************** 
*Name: memdev.c 
*Desc: 字符设备驱动程序的框架结构
*Parameter:  
*Return: 
*Author: derek 
*Date: 2013-6-4 
********************************************************************************/  
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/module.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/cdev.h>  

//module_param(dev_major, int, S_IRUGO);  
#define MY_DEV_MAJOR        260     /*预设的mem的主设备号*/  
static int dev_major = 0; //MY_DEV_MAJOR;  

#define MY_DEV_NR_DEVS      2       /*设备数*/  

#define CHAR_DEV_NAME       "kpipe"
  
/*
'k'为幻数，要按照Linux内核的约定方法为驱动程序选择ioctl编号，
应该首先看看include/asm/ioctl.h和Documentation/ioctl-number.txt这两个文件.

对幻数的编号千万不能重复定义，如ioctl-number.txt已经说明‘k'的编号已经被占用的范围为：
'k'    00-0F    linux/spi/spidev.h    conflict!
'k'    00-05    video/kyro.h        conflict!
所以我们在这里分别编号为0x1a和0x1b
*/
#define CMD_MAGIC   'k'
#define MEM_CMD1    _IO(CMD_MAGIC, 0x1a)
#define MEM_CMD2    _IO(CMD_MAGIC, 0x1b)
#define MEM_CMD3    _IO(CMD_MAGIC, 0x1c)

#if 1
#define MAX_UID_NUM     32

typedef struct tagUSER_MSG_INFO
{
    int  uid;
    int  param1;
    int  param2;
    int  param3;
    char *msg;
}USER_MSG_INFO;

typedef struct tagUSER_QUEUE_INFO
{
    int  rd_ptr;
    int  wt_ptr;
    int  max_msg;
    int  msg_max_len;
    char *msg_queue;
}USER_QUEUE_INFO;

USER_QUEUE_INFO usr_queue[MAX_UID_NUM];

static int user_queue_init(void)
{
    int i;

    for(i = 0; i < MAX_UID_NUM; i++) {
        memset(&usr_queue[i], 0, sizeof(USER_QUEUE_INFO));
    }
    return 0;
}

static int user_queue_alloc(int uid, int max_msg, int msg_max_len)
{
    int total_size;
    
    if (uid >= MAX_UID_NUM) return -1;
    if (max_msg < 1 || msg_max_len < 1) return -1;
    if (usr_queue[uid].msg_queue != NULL) return -2;

    total_size = max_msg * msg_max_len;
    usr_queue[uid].msg_queue = kmalloc(total_size, GFP_KERNEL);  
    if (NULL == usr_queue[uid].msg_queue) return -3;

    usr_queue[uid].max_msg = max_msg;
    usr_queue[uid].msg_max_len = msg_max_len;
    usr_queue[uid].rd_ptr = 0;
    usr_queue[uid].wt_ptr = 0;
    
    return 0;
}

static int user_queue_push(USER_MSG_INFO *usr_msg)
{
    int uid;
    char *ptr;
    
    if (NULL == usr_msg) return -1;
    uid = usr_msg->uid;
    if (usr_queue[uid].msg_queue == NULL) return -2;

    if ( (usr_queue[uid].wt_ptr + 1) % usr_queue[uid].max_msg == usr_queue[uid].rd_ptr)
        return -3; // full

    ptr = usr_queue[uid].msg_queue + usr_queue[uid].wt_ptr * usr_queue[uid].msg_max_len;
    memset(ptr, 0, usr_queue[uid].msg_max_len);
    copy_from_user((int *)ptr, (int *)usr_msg->msg, usr_queue[uid].msg_max_len);
    usr_queue[uid].wt_ptr = (usr_queue[uid].wt_ptr + 1) % usr_queue[uid].max_msg;
        
    return 0;
}

static int user_queue_pop(USER_MSG_INFO *usr_msg)
{
    int uid;
    char *ptr;
    
    if (NULL == usr_msg) return -1;
    uid = usr_msg->uid;
    if (usr_queue[uid].msg_queue == NULL) return -2;

    if ( usr_queue[uid].rd_ptr == usr_queue[uid].wt_ptr)
        return -3; // empty

    ptr = usr_queue[uid].msg_queue + usr_queue[uid].rd_ptr * usr_queue[uid].msg_max_len;
    copy_to_user((int *)usr_msg->msg, (int *)ptr, usr_queue[uid].msg_max_len);
    usr_queue[uid].rd_ptr = (usr_queue[uid].rd_ptr + 1) % usr_queue[uid].max_msg;
        
    return 0;
}

static int user_queue_destroy(void)
{
    int i;

    for(i = 0; i < MAX_UID_NUM; i++) {
        usr_queue[i].max_msg = 0;
        if (NULL != usr_queue[i].msg_queue)
            kfree(usr_queue[i].msg_queue);
        usr_queue[i].msg_queue = NULL;
    }
    return 0;
}


#endif

#if 1
int my_dev_open(struct inode *inode, struct file *filp)  
{  
    return 0;   
}  
  
int my_dev_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
static int my_dev_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{    
    USER_MSG_INFO user_msg;
    int ret = 0;
    
    switch(cmd)
    {
        case MEM_CMD1: // init mq for uid
            if(copy_from_user(&user_msg,  (int *)arg, sizeof(USER_MSG_INFO))) 
                return -EFAULT;
            
            ret = user_queue_alloc(user_msg.uid, user_msg.param1, user_msg.param2);
            break;
            
        case MEM_CMD2: // send msg to uid
            if(copy_from_user(&user_msg,  (int *)arg, sizeof(USER_MSG_INFO))) 
                return -EFAULT;

            ret = user_queue_push(&user_msg);
            break;
        
        case MEM_CMD3: // get msg from uid
            if(copy_from_user(&user_msg,  (int *)arg, sizeof(USER_MSG_INFO))) 
                return -EFAULT;
            
            ret = user_queue_pop(&user_msg);
            break;
    }
    
    return ret;
}

static const struct file_operations my_dev_fops =  
{  
    .owner = THIS_MODULE,  
    .unlocked_ioctl = my_dev_ioctl,
    .open = my_dev_open,  
    .release = my_dev_release,  
};  

struct class *pclass = NULL;  
  
struct cdev my_dev;   

static int my_dev_init(void)  
{  
    int result;  
    dev_t devno = MKDEV(dev_major, 0);  

    if (dev_major) { /* 静态申请设备号*/  
        result = register_chrdev_region(devno, 2, CHAR_DEV_NAME);  
    } else { /* 动态分配设备号 */  
        result = alloc_chrdev_region(&devno, 0, 2, CHAR_DEV_NAME);  
        dev_major = MAJOR(devno);  
    }   

    if (result < 0)  
        return result;  

    cdev_init(&my_dev, &my_dev_fops);  
    my_dev.owner = THIS_MODULE;  
    my_dev.ops = &my_dev_fops;  
    cdev_add(&my_dev, MKDEV(dev_major, 0), MY_DEV_NR_DEVS);  

    pclass = class_create(THIS_MODULE, CHAR_DEV_NAME);  
    if (IS_ERR(pclass))  
    {  
        printk("class_create failed!/n");  
        goto failed;  
    }  

    user_queue_init();
    device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    return 0;  

failed:   
    cdev_del(&my_dev);
    unregister_chrdev_region(devno, 1);  
    return result;  
}  
  
static void my_dev_exit(void)  
{  
    device_destroy(pclass, MKDEV(dev_major, 0));  
    class_destroy(pclass);
    
    cdev_del(&my_dev);
    unregister_chrdev_region(MKDEV(dev_major, 0), 2); 
    user_queue_destroy();
}  

#endif

MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  
  
module_init(my_dev_init);  
module_exit(my_dev_exit);  

