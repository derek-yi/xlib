
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/file.h>
#include <linux/suspend.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/regmap.h>


static struct i2c_adapter *adap;
static struct i2c_client  *client;

#define  I2C_DEVICE_ADDR   0x68

static struct i2c_board_info downey_board = {
    I2C_BOARD_INFO("downey_i2c", I2C_DEVICE_ADDR),
};

extern struct i2c_client *
i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info);

int dev_init(void)
{
    adap = i2c_get_adapter(2);
    if (IS_ERR(adap)){
        printk(KERN_ALERT  "I2c_get_adapter failed!!!\n");
        return -ENODEV;
    }
    
    client = i2c_new_device(adap, &downey_board);
    
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

