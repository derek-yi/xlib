
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/regmap.h>
//#include <linux/paltform_device.h>


static struct i2c_adapter *adap;
static struct i2c_client  *client;

#define  I2C_DEVICE_ADDR   0x68

/**指定i2c device的信息
 * downey_i2c 是device中的name元素，当这个模块被加载时，i2c总线将使用这个名称匹配相应的drv。
 * I2C_DEVICE_ADDR  为设备的i2c地址 
 * */
static struct i2c_board_info downey_board = {
    I2C_BOARD_INFO("downey_i2c", I2C_DEVICE_ADDR),
};


int dev_init(void)
{
    /*获取i2c适配器，适配器一般指板上I2C控制器，实现i2c底层协议的字节收发，特殊情况下，用普通gpio模拟I2C也可作为适配器*/
    adap = i2c_get_adapter(2);
    if (IS_ERR(adap)){
        printk(KERN_ALERT  "I2c_get_adapter failed!!!\n");
        return -ENODEV;
    }
    
    /*创建一个I2C device，并注册到i2c bus的device链表中*/
    client = i2c_new_device(adap, &downey_board);
    
    /*使能相应适配器*/
    i2c_put_adapter(adap);
    if (!client) {
        printk(KERN_ALERT  "Get new device failed!!!\n");
        return -ENODEV;
    }
    
    return 0;
}

void dev_exit(void)
{
    i2c_unregister_device(client);
    return ;
}

MODULE_LICENSE("GPL");
module_init(dev_init);
module_exit(dev_exit);

