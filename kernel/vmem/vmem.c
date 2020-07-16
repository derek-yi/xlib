
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/poll.h>


//#undef DEBUG
#define DEBUG
#ifdef DEBUG
#define dprintk(fmt, arg...) printk(KERN_DEBUG fmt, ##arg)
#else
#define dprintk(...) do { } while (0)
#endif

static struct class *vmem_class = NULL;

static int vmem_major = 0;        //模块vmem_major参数，默认为0
static int vmem_minor = 0;
module_param(vmem_major, int, S_IRUGO);

static int dev_num = 1;            //模块dev_num参数，默认为1
module_param(dev_num, int, S_IRUGO);


struct vmem_dev {
    unsigned int current_len;
    unsigned char mem[PAGE_SIZE];
    struct cdev cdev;
    struct mutex mutex;
    struct device_attribute device_attribute;
    wait_queue_head_t r_wait;
    wait_queue_head_t w_wait;
    struct fasync_struct *async_queue;

};
static struct  vmem_dev *vmem_devp;


ssize_t virtual_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct  vmem_dev *devp = (struct vmem_dev *)dev_get_drvdata(dev);
    mutex_lock(&devp->mutex);
    snprintf(buf, devp->current_len + 1, "%s", devp->mem);
    mutex_unlock(&devp->mutex);
    return devp->current_len + 1;
}

ssize_t virtual_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct  vmem_dev *devp = (struct vmem_dev *)dev_get_drvdata(dev);
    mutex_lock(&devp->mutex);
    if (PAGE_SIZE == devp->current_len) {
        devp->current_len = 0;
        return -1;
    }
    memcpy(devp->mem, buf, count);
    devp->current_len = count;
    mutex_unlock(&devp->mutex);
    return count;
}

static int virtual_open(struct inode *inode, struct file *filp)
{
    struct  vmem_dev *devp;
    devp = container_of(inode->i_cdev, struct vmem_dev, cdev);
    filp->private_data = devp;
    return 0;
}

static ssize_t virual_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret;
    struct vmem_dev *devp = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);

    mutex_lock(&devp->mutex);
    add_wait_queue(&devp->r_wait, &wait);        //定义读队列唤醒

    while (!devp->current_len) {
        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto out1;
        }

        set_current_state(TASK_INTERRUPTIBLE);    //读阻塞休眠
        mutex_unlock(&devp->mutex);
        schedule();
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto out2;
        }
        mutex_lock(&devp->mutex);
    }

    if (count > devp->current_len)
        count = devp->current_len;
    if (copy_to_user(buf, devp->mem, count)) {
        ret = -EFAULT;
        goto out1;
    } else {
        devp->current_len -= count;
        memcpy(devp->mem, devp->mem + count, devp->current_len);
        dprintk("read %d bytes,current_len:%d\n", (int)count, devp->current_len);
        wake_up_interruptible(&devp->w_wait);        //唤醒读阻塞休眠
        ret = count;
    }
out1:
    mutex_unlock(&devp->mutex);
out2:
    remove_wait_queue(&devp->r_wait, &wait);
    set_current_state(TASK_RUNNING);
    return ret;
}

static ssize_t virual_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret;
    struct vmem_dev *devp = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);

    mutex_lock(&devp->mutex);
    add_wait_queue(&devp->w_wait, &wait);        //定义写队列唤醒

    while (devp->current_len == PAGE_SIZE) {
        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto out1;
        }

        set_current_state(TASK_INTERRUPTIBLE);    //写阻塞休眠
        mutex_unlock(&devp->mutex);
        schedule();
        if (signal_pending(current)) {
            ret = -ERESTARTSYS;
            goto out2;
        }
        mutex_lock(&devp->mutex);
    }
    
    if (count > PAGE_SIZE - devp->current_len)
        count = PAGE_SIZE - devp->current_len;
    if (copy_from_user(devp->mem + devp->current_len, buf, count)) {
        ret = -EFAULT;
        goto out1;
    } else {
        devp->current_len += count;
        dprintk("written %d bytes,current_len:%d\n", (int)count, devp->current_len);
        wake_up_interruptible(&devp->r_wait);        //唤醒读阻塞休眠

        if (devp->async_queue) {    //写信号进行异步通知应用
            kill_fasync(&devp->async_queue, SIGIO, POLL_IN);
            dprintk("%s kill SIGIO\n", __func__);
        }
        ret = count;
    }
    
out1:
    mutex_unlock(&devp->mutex);
out2:
    remove_wait_queue(&devp->w_wait, &wait);
    set_current_state(TASK_RUNNING);
    return ret;
}

//这里利用IO CMD宏定义
#define MEM_CLEAR           _IO('V',1)
#define MEM_FULL            _IOW('V', 2, unsigned char)

static long virtual_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct vmem_dev *devp = filp->private_data;
    
    switch(cmd) {
    case MEM_CLEAR:
        mutex_lock(&devp->mutex);
        devp->current_len = 0;
        memset(devp->mem, 0, PAGE_SIZE);
        mutex_unlock(&devp->mutex);
        dprintk("cmd = %d, clear the memory", cmd);
        break;
        
    case MEM_FULL:
        mutex_lock(&devp->mutex);
        devp->current_len = PAGE_SIZE;
        memset(devp->mem, (unsigned char )arg, PAGE_SIZE);
        mutex_unlock(&devp->mutex);
        dprintk("cmd = %d, fill the memory using %ld", cmd, arg);
        break;
        
    default:
        return -EINVAL;
    }
    
    return 0;
}

static unsigned int virtual_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    struct vmem_dev *devp = filp->private_data;
    
    mutex_lock(&devp->mutex);
    poll_wait(filp, &devp->r_wait, wait);    //声明读写队列到poll table唤醒线程
    poll_wait(filp, &devp->w_wait, wait);

    if (devp->current_len != 0 )
        mask |= POLLIN | POLLRDNORM;

    if (devp->current_len != PAGE_SIZE)
        mask |= POLLOUT | POLLWRNORM;

    mutex_unlock(&devp->mutex);
    return mask;
}

static int virtual_fasync(int fd, struct file *filp, int mode)
{
    struct vmem_dev *devp = filp->private_data;
    return fasync_helper(fd, filp, mode, &devp->async_queue);
}

static int virtual_release(struct inode *inode, struct file *filp)
{
    virtual_fasync(-1, filp, 0);
    return 0;
}


static  struct file_operations virtual_fops = {
    .owner = THIS_MODULE,
    .read = virual_read,
    .write = virual_write,
    .unlocked_ioctl = virtual_ioctl,
    .poll = virtual_poll,
    .fasync = virtual_fasync,
    .open = virtual_open,
    .release = virtual_release,
};

static int virtualmem_setup_dev(struct vmem_dev *devp, int index)
{
    int ret;
    dev_t devno = MKDEV(vmem_major, vmem_minor + index);

    cdev_init(&devp->cdev, &virtual_fops);
    devp->cdev.owner = THIS_MODULE;
    ret = cdev_add(&devp->cdev, devno, 1);
    return ret;
}


static int __init virtualmem_init(void)
{
    int ret;
    int i;
    dev_t devno = MKDEV(vmem_major, vmem_minor);
    struct device *temp[dev_num];

    dprintk("Initializing virtualmem device.\n");
    if(vmem_major)
        ret = register_chrdev_region(devno, dev_num, "vmem");
    else {
        ret = alloc_chrdev_region(&devno, 0, dev_num, "vmem");
        vmem_major = MAJOR(devno);
        vmem_minor = MINOR(devno);
    }
    if(ret < 0) {
        dprintk("Failed to alloc char dev region.\n");
        goto err;
    }

    vmem_devp = kzalloc(sizeof(struct vmem_dev) * dev_num, GFP_KERNEL);
    if (!vmem_devp) {
        ret = -ENOMEM;
        dprintk("Failed to alloc virtualmem device.\n");
        goto unregister;
    }

    for(i = 0; i < dev_num; i++) {
        ret = virtualmem_setup_dev(vmem_devp + i, i);
        if (ret) {
            dprintk("Failed to setup dev %d.\n", i);
            while(--i >= 0)
                cdev_del(&(vmem_devp + i)->cdev);
            goto kfree;
        }
    }

    vmem_class = class_create(THIS_MODULE, "virtualmem");    //建立virtualmem类
    if (IS_ERR(vmem_class)) {
        ret = PTR_ERR(vmem_class);
        dprintk("Failed to create virtualmem class.\n");
        goto destroy_cdev;
    }

    for(i = 0; i < dev_num; i++)  {
        //在virtualmem这个类里建立多个virtualmem文件
        temp[i] = device_create(vmem_class, NULL, devno + i, (void *)(vmem_devp + i), "%s%d", "virtualmem", i);
        if (IS_ERR(temp[i])) {
            ret = PTR_ERR(temp[i]);
            dprintk("Failed to create virtualmem device.\n");
            while(--i >= 0)
                device_destroy(vmem_class, devno + i);
            goto destory_class;
        }
    }
    
    for(i = 0; i < dev_num; i++) {
        (vmem_devp + i)->device_attribute.attr.name = "mem";    //对于单设备一般用宏 DEVICE_ATTR，这里多设备需要完成宏的代码
        (vmem_devp + i)->device_attribute.attr.mode = S_IRUGO | S_IWUSR;
        (vmem_devp + i)->device_attribute.show = virtual_show;
        (vmem_devp + i)->device_attribute.store = virtual_store;
        ret =  device_create_file(temp[i], &(vmem_devp + i)->device_attribute);
        if(ret < 0) {
            dprintk("Failed to create attribute mem.");
            while(--i >= 0)
                device_remove_file(temp[i], &(vmem_devp + i)->device_attribute);
            goto destroy_device;
        }
    }
    
    for(i = 0; i < dev_num; i++)  {
        mutex_init(&(vmem_devp + i)->mutex);    //初始化互斥锁
        init_waitqueue_head(&(vmem_devp + i)->r_wait);        //初始化读写队列
        init_waitqueue_head(&(vmem_devp + i)->w_wait);
    }

    dprintk("Success to Initialize virtualmem device.\n");
    return 0;

destroy_device:
    for(i = 0; i < dev_num; i++)
        device_destroy(vmem_class, devno + i);

destory_class:
    class_destroy(vmem_class);

destroy_cdev:
    for(i = 0; i < dev_num; i++)
        cdev_del(&(vmem_devp + i)->cdev);

kfree:
    kfree(vmem_devp);

unregister:
    unregister_chrdev_region(MKDEV(vmem_major, vmem_minor), dev_num);

err:
    return ret;
}

static void __exit virualmem_exit(void)
{
    int i;
    
    dprintk("Destroy virtualmem device.\n");
    if(vmem_class) {
        for(i = 0; i < dev_num; i++)
            device_destroy(vmem_class, MKDEV(vmem_major, vmem_minor + i));
        class_destroy(vmem_class);
    }
    
    if(vmem_devp) {
        for(i = 0; i < dev_num; i++)
            cdev_del(&(vmem_devp + i)->cdev);
        kfree(vmem_devp);
    }
    
    unregister_chrdev_region(MKDEV(vmem_major, vmem_minor), dev_num);
}

module_init(virtualmem_init);
module_exit(virualmem_exit);

MODULE_AUTHOR("Kevin Hwang <kevin.hwang@live.com");
MODULE_LICENSE("GPL v2");

