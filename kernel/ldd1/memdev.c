/****************************************************************************** 
*Name: memdev.c 
*Desc: �ַ��豸��������Ŀ�ܽṹ�����ַ��豸������һ����ʵ�������豸�� 
*      ����ʹ���ڴ���ģ��һ���ַ��豸 
* Parameter:  
* Return: 
* Author: derek 
* Date: 2013-6-4 
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
  
#define MEMDEV_MAJOR        260     /*Ԥ���mem�����豸��*/  
#define MEMDEV_NR_DEVS      2       /*�豸��*/  
#define MEMDEV_SIZE         4096  
#define CHAR_DEV_NAME       "memdev"
  
/* mem�豸�����ṹ�� */  
struct mem_dev   
{   
  char *data;   
  unsigned long size;   
};  
  
static int mem_major = MEMDEV_MAJOR;  
  
module_param(mem_major, int, S_IRUGO);  
  
struct mem_dev *mem_devp; /* �豸�ṹ��ָ�� */  
  
struct cdev cdev;   
  
int mem_open(struct inode *inode, struct file *filp)  
{  
    struct mem_dev *dev;  
      
    /*��ȡ���豸��*/  
    int num = MINOR(inode->i_rdev);  

    if (num >= MEMDEV_NR_DEVS)   
            return -ENODEV;  
    dev = &mem_devp[num];  
      
    /*���豸�����ṹָ�븳ֵ���ļ�˽������ָ��*/  
    filp->private_data = dev;  
      
    return 0;   
}  
  
int mem_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
  
static ssize_t mem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)  
{  
    unsigned long p = *ppos;  
    unsigned int count = size;  
    int ret = 0;  
    struct mem_dev *dev = filp->private_data; /*����豸�ṹ��ָ��*/  

    /*�ж϶�λ���Ƿ���Ч*/  
    if (p >= MEMDEV_SIZE)  
        return 0;  
    if (count > MEMDEV_SIZE - p)  
        count = MEMDEV_SIZE - p;  

    /*�����ݵ��û��ռ�*/  
    if (copy_to_user(buf, (void*)(dev->data + p), count))  
    {  
        ret = - EFAULT;  
    }  
    else  
    {  
        *ppos += count;  
        ret = count;  
        printk(KERN_INFO "read %d bytes(s) from %d\n", count, p);  
    }  

    return ret;  
}  
  
static ssize_t mem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)  
{  
    unsigned long p = *ppos;  
    unsigned int count = size;  
    int ret = 0;  
    struct mem_dev *dev = filp->private_data; /*����豸�ṹ��ָ��*/  

    /*�����ͻ�ȡ��Ч��д����*/  
    if (p >= MEMDEV_SIZE)  
        return 0;  
    if (count > MEMDEV_SIZE - p)  
        count = MEMDEV_SIZE - p;  

    /*���û��ռ�д������*/  
    if (copy_from_user(dev->data + p, buf, count))  
        ret = - EFAULT;  
    else  
    {  
        *ppos += count;  
        ret = count;  
        printk(KERN_INFO "written %d bytes(s) from %d\n", count, p);  
    }  

    return ret;  
}  
  
static loff_t mem_llseek(struct file *filp, loff_t offset, int whence)  
{   
    loff_t newpos;  
  
    switch(whence) {  
      case 0: /* SEEK_SET */  
        newpos = offset;  
        break;  
  
      case 1: /* SEEK_CUR */  
        newpos = filp->f_pos + offset;  
        break;  
  
      case 2: /* SEEK_END */  
        newpos = MEMDEV_SIZE -1 + offset;  
        break;  
  
      default: /* can't happen */  
        return -EINVAL;  
    }  
    if ((newpos<0) || (newpos>MEMDEV_SIZE))  
     return -EINVAL;  
       
    filp->f_pos = newpos;  
    return newpos;  
}  
  
/*�ļ������ṹ��*/  
static const struct file_operations mem_fops =  
{  
    .owner = THIS_MODULE,  
    .llseek = mem_llseek,  
    .read = mem_read,  
    .write = mem_write,  
    .open = mem_open,  
    .release = mem_release,  
};  

struct class *pclass;  

/*�豸����ģ����غ���*/  
static int memdev_init(void)  
{  
    int result;  
    int i;  

    dev_t devno = MKDEV(mem_major, 0);  

    if (mem_major)  { /* ��̬�����豸��*/  
        result = register_chrdev_region(devno, 2, CHAR_DEV_NAME);  
    } else {/* ��̬�����豸�� */  
        result = alloc_chrdev_region(&devno, 0, 2, CHAR_DEV_NAME);  
        mem_major = MAJOR(devno);  
    }   

    if (result < 0)  
        return result;  

    /*��ʼ��cdev�ṹ*/  
    cdev_init(&cdev, &mem_fops);  
    cdev.owner = THIS_MODULE;  
    cdev.ops = &mem_fops;  

    /* ע���ַ��豸 */  
    cdev_add(&cdev, MKDEV(mem_major, 0), MEMDEV_NR_DEVS);  

    /* Ϊ�豸�����ṹ�����ڴ�*/  
    mem_devp = kmalloc(MEMDEV_NR_DEVS * sizeof(struct mem_dev), GFP_KERNEL);  
    if (!mem_devp) /*����ʧ��*/  
    {  
        result = - ENOMEM;  
        goto fail_malloc;  
    }  
    memset(mem_devp, 0, sizeof(struct mem_dev));  

    /*Ϊ�豸�����ڴ�*/  
    for (i=0; i < MEMDEV_NR_DEVS; i++)   
    {  
        mem_devp[i].size = MEMDEV_SIZE;  
        mem_devp[i].data = kmalloc(MEMDEV_SIZE, GFP_KERNEL);  
        memset(mem_devp[i].data, 0, MEMDEV_SIZE);  
    }  

    pclass = class_create(THIS_MODULE, CHAR_DEV_NAME);  
    if (IS_ERR(pclass))  
    {  
        printk("class_create failed!/n");  
        kfree(mem_devp);
        goto fail_malloc;  
    }  

    device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    return 0;  

fail_malloc:   
    unregister_chrdev_region(devno, 1);  
    return result;  
}  
  
/*ģ��ж�غ���*/  
static void memdev_exit(void)  
{  
    device_destroy(pclass, MKDEV(mem_major, 0));  
    class_destroy(pclass);
    cdev_del(&cdev); /*ע���豸*/  
    kfree(mem_devp); /*�ͷ��豸�ṹ���ڴ�*/  
    unregister_chrdev_region(MKDEV(mem_major, 0), 2); /*�ͷ��豸��*/  
}  
  
MODULE_AUTHOR("derek yi");  
MODULE_LICENSE("GPL");  
  
module_init(memdev_init);  
module_exit(memdev_exit);  
