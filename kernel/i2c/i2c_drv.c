
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/* 结构体数组 结构体第一个参数为名称，第二个参数为private数据*/
static const struct i2c_device_id downey_drv_id_table[] = {
    {"downey_i2c", 0},
    {},
};

static int major;
static struct class *i2c_test_cls;
static struct device *i2c_test_dev;
static const char* CLASS_NAME = "I2C_TEST_CLASS";
static const char* DEVICE_NAME = "I2C_TEST_DEVICE";

static struct i2c_client *downey_client;


static int i2c_test_open(struct inode *node, struct file *file)
{
    printk(KERN_ALERT "i2c init \n");
    return 0;
}

static ssize_t i2c_test_read(struct file *file,char *buf, size_t len,loff_t *offset)
{
    int cnt = 0;
    uint8_t reg = 0;
    uint8_t val = 0;
    
    copy_from_user(&reg, buf, 1);
    /*i2c读byte，通过这个函数可以从设备中指定地址读取数据*/
    val = i2c_smbus_read_byte_data(downey_client, reg);
    cnt = copy_to_user(&buf[1], &val, 1);
    
    return 1;
}

static ssize_t i2c_test_write(struct file *file,const char *buf,size_t len,loff_t *offset)
{
    uint8_t recv_msg[255] = {0};
    uint8_t reg = 0;
    int cnt = 0;
    
    cnt = copy_from_user(recv_msg, buf, len);
    reg = recv_msg[0];
    printk(KERN_INFO "recv data = %x.%x\n",recv_msg[0],recv_msg[1]);
    
    /*i2c写byte，通过这个函数可以往设备中指定地址写数据*/
    if(i2c_smbus_write_byte_data(downey_client, reg, recv_msg[1]) < 0){
        printk(KERN_ALERT  " write failed!!!\n");
        return -EIO;
    }
    
    return len;
}

static int i2c_test_release(struct inode *node,struct file *file)
{
    printk(KERN_INFO "Release!!\n");
    
    return 0;
}

static struct file_operations file_oprts = 
{
    .open = i2c_test_open,
    .read = i2c_test_read,
    .write = i2c_test_write,
    .release = i2c_test_release,
};

/*当i2c bus检测到匹配的device - driver，调用probe()函数，在probe函数中，申请设备号，创建设备节点，绑定相应的file operation结构体。*/
static int downey_drv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    /*保存参数client，在i2c读写操作时需要用到这个参数，其中保存了适配器、设备地址等信息*/
    printk(KERN_ALERT "addr = %x\n",client->addr);
    downey_client = client;
    major = register_chrdev(0,DEVICE_NAME,&file_oprts);
    if(major < 0 ){
        printk(KERN_ALERT "Register failed!!\r\n");
        return major;
    }
    printk(KERN_ALERT "Registe success,major number is %d\r\n",major);

    /*以CLASS_NAME创建一个class结构，这个动作将会在/sys/class目录创建一个名为CLASS_NAME的目录*/
    i2c_test_cls = class_create(THIS_MODULE,CLASS_NAME);
    if(IS_ERR(i2c_test_cls))
    {
        unregister_chrdev(major,DEVICE_NAME);
        return PTR_ERR(i2c_test_cls);
    }

    /*以DEVICE_NAME为名，参考/sys/class/CLASS_NAME在/dev目录下创建一个设备：/dev/DEVICE_NAME*/
    i2c_test_dev = device_create(i2c_test_cls,NULL,MKDEV(major,0),NULL,DEVICE_NAME);
    if(IS_ERR(i2c_test_dev))
    {
        class_destroy(i2c_test_cls);
        unregister_chrdev(major,DEVICE_NAME);
        return PTR_ERR(i2c_test_dev);
    }
    
    printk(KERN_ALERT "i2c_test device init success!!\r\n");
    return 0;
}

/*Remove :当匹配关系不存在时(device或是driver被卸载)，调用remove函数，remove函数是probe函数的反操作，将probe函数中申请的资源全部释放。*/
static int downey_drv_remove(struct i2c_client *client)
{
    printk(KERN_ALERT  "remove!!!\n");
    device_destroy(i2c_test_cls,MKDEV(major,0));
    class_unregister(i2c_test_cls);
    class_destroy(i2c_test_cls);
    unregister_chrdev(major,DEVICE_NAME);
    return 0;
}

static struct i2c_driver downey_drv = {
    /*.driver中的name元素仅仅是一个标识，并不作为bus匹配的name识别*/
    .driver = {
        .name = "random",
        .owner = THIS_MODULE,
    },
    .probe = downey_drv_probe,
    .remove = downey_drv_remove,
    /*.id_table中存储driver名称，作为bus匹配时的识别*/
    .id_table = downey_drv_id_table,
    // .address_list = downey_i2c,
};


int drv_init(void)
{
    int ret = 0;
    
    printk(KERN_ALERT  "drv_init!!!\n");
    ret = i2c_add_driver(&downey_drv);
    if(ret){
        printk(KERN_ALERT "add driver failed!!!\n");
        return -ENODEV;
    }
    
    return 0;
}


void drv_exit(void)
{
    i2c_del_driver(&downey_drv);
    return ;
}

MODULE_LICENSE("GPL");
module_init(drv_init);
module_exit(drv_exit);

