
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

/**ָ��i2c device����Ϣ
 * downey_i2c ��device�е�nameԪ�أ������ģ�鱻����ʱ��i2c���߽�ʹ���������ƥ����Ӧ��drv��
 * I2C_DEVICE_ADDR  Ϊ�豸��i2c��ַ 
 * */
static struct i2c_board_info downey_board = {
    I2C_BOARD_INFO("downey_i2c", I2C_DEVICE_ADDR),
};


int dev_init(void)
{
    /*��ȡi2c��������������һ��ָ����I2C��������ʵ��i2c�ײ�Э����ֽ��շ�����������£�����ͨgpioģ��I2CҲ����Ϊ������*/
    adap = i2c_get_adapter(2);
    if (IS_ERR(adap)){
        printk(KERN_ALERT  "I2c_get_adapter failed!!!\n");
        return -ENODEV;
    }
    
    /*����һ��I2C device����ע�ᵽi2c bus��device������*/
    client = i2c_new_device(adap, &downey_board);
    
    /*ʹ����Ӧ������*/
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

