/****************************************************************************** 
* Name: memdev.c 
* Desc: �ַ��豸��������Ŀ�ܽṹ
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

//module_param(mem_major, int, S_IRUGO);  

#define MEMDEV_MAJOR        260     /*Ԥ���mem�����豸��*/  
#define MEMDEV_NR_DEVS      2       /*�豸��*/  
#define MEMDEV_SIZE         4096  
#define CHAR_DEV_NAME       "memdev"
  
static int mem_major = 0; //MEMDEV_MAJOR;  

struct class *pclass = NULL;  
  
struct cdev my_dev;   
  
int mem_open(struct inode *inode, struct file *filp)  
{  
    return 0;   
}  
  
int mem_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  

/*
'k'Ϊ������Ҫ����Linux�ں˵�Լ������Ϊ��������ѡ��ioctl��ţ�
Ӧ�����ȿ���include/asm/ioctl.h��Documentation/ioctl-number.txt�������ļ�.

�Ի����ı��ǧ�����ظ����壬��ioctl-number.txt�Ѿ�˵����k'�ı���Ѿ���ռ�õķ�ΧΪ��
'k'    00-0F    linux/spi/spidev.h    conflict!
'k'    00-05    video/kyro.h        conflict!
��������������ֱ���Ϊ0x1a��0x1b
*/
#define CMD_MAGIC   'k'
#define MEM_CMD1    _IO(CMD_MAGIC, 0x1a)
#define MEM_CMD2    _IO(CMD_MAGIC, 0x1b)

int temp_data = 0;

static int mem_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{    
    switch(cmd)
    {
        case MEM_CMD1:
            if(copy_from_user(&temp_data,  (int *)arg, sizeof(int))) 
                return -EFAULT;
            break;
        
        case MEM_CMD2:
            if(copy_to_user( (int *)arg, &temp_data, sizeof(int))) 
                return -EFAULT;
            break;
    }
    
    //printk(KERN_NOTICE"ioctl CMD%d done!\n",temp);    
    return 0;
}

static const struct file_operations mem_fops =  
{  
    .owner = THIS_MODULE,  
    .unlocked_ioctl = mem_ioctl,
    .open = mem_open,  
    .release = mem_release,  
};  

static int memdev_init(void)  
{  
    int result;  
    int i;  

    dev_t devno = MKDEV(mem_major, 0);  

    if (mem_major) { /* ��̬�����豸��*/  
        result = register_chrdev_region(devno, 2, CHAR_DEV_NAME);  
    } else { /* ��̬�����豸�� */  
        result = alloc_chrdev_region(&devno, 0, 2, CHAR_DEV_NAME);  
        mem_major = MAJOR(devno);  
    }   

    if (result < 0)  
        return result;  

    cdev_init(&my_dev, &mem_fops);  
    my_dev.owner = THIS_MODULE;  
    my_dev.ops = &mem_fops;  
    cdev_add(&my_dev, MKDEV(mem_major, 0), MEMDEV_NR_DEVS);  

    pclass = class_create(THIS_MODULE, CHAR_DEV_NAME);  
    if (IS_ERR(pclass))  
    {  
        printk("class_create failed!/n");  
        goto failed;  
    }  

    device_create(pclass, NULL, devno, NULL, CHAR_DEV_NAME);  
    return 0;  

failed:   
    cdev_del(&my_dev);
    unregister_chrdev_region(devno, 1);  
    return result;  
}  
  
static void memdev_exit(void)  
{  
    device_destroy(pclass, MKDEV(mem_major, 0));  
    class_destroy(pclass);
    
    cdev_del(&my_dev);
    unregister_chrdev_region(MKDEV(mem_major, 0), 2); 
}  
  
MODULE_AUTHOR("derek yi");  
MODULE_LICENSE("GPL");  
  
module_init(memdev_init);  
module_exit(memdev_exit);  

