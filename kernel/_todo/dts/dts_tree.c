
#if xxx
static inline bool of_property_read_bool(const struct device_node *np, 
                        const char *propname)

static inline int of_property_read_u8(const struct device_node *np, 
                        const char *propname, u8 *out_value)

static inline int of_property_read_u16(const struct device_node *np, 
                        const char *propname, u16 *out_value)

static inline int of_property_read_u32(const struct device_node *np, 
                        const char *propname, u32 *out_value)

static inline int of_property_read_string_array(struct device_node *np,
                        const char *propname, const char **out_strs,
                        size_t sz)

static inline int of_property_count_strings(struct device_node *np,
                        const char *propname)

static inline int of_property_read_string_index(struct device_node *np,
                        const char *propname,
                        int index, const char **output)

//dts文件以“/”为根目录，以树形展开，要注意思大括号的匹配。
/* add by song for dvb widgets */
    dvb_widgets {
        compatible = "amlogic, dvb_widgets"; //platform_driver 指定的匹配表。
        status = "okay"; //设备状态
        dw_name = "dvb-widgets"; //字符串属性
        dw_num = <8>; //数值属性
        ant_power-gpio = <&gpio  GPIODV_15  GPIO_ACTIVE_HIGH>; //gpio 描述
        loops-gpio = <&gpio  GPIODV_13  GPIO_ACTIVE_HIGH>; //gpio 描述
        ch34-gpio = <&gpio  GPIODV_12  GPIO_ACTIVE_HIGH>; //gpio 描述
    };

#endif


#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/device.h>

#include <asm/irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <uapi/linux/input.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/gpio-amlogic.h>
#include <linux/amlogic/sd.h>
#include <linux/amlogic/iomap.h>
#include <dt-bindings/gpio/gxbb.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/pm.h>
#include <linux/of_address.h>

struct dw_dev {
    unsigned int ant_power_pin;
    unsigned int ant_overload_pin;
    unsigned int loops_pin;
    unsigned int ch3_4_pin;
};

struct dw_dev pdw_dev;


static int dvb_widgets_suspend(struct platform_device *pdev,  pm_message_t pm_status)
{
    pr_dbg("%s\n",__FUNCTION__);
    return 0;
}

static int dvb_widgets_resume(struct platform_device *pdev)
{
    pr_dbg("%s\n",__FUNCTION__);
    return 0;
}

static void dvb_widgets_shutdown(struct platform_device *pdev)
{
    pr_dbg("%s\n",__FUNCTION__);
}


static int dvb_widgets_remove(struct platform_device *pdev)
{
    pr_dbg("%s\n",__FUNCTION__);

#ifdef D_SUPPORT_CLASS_INTERFACE    
    class_unregister(&dvb_widgets_class);
#endif

    return 0;
}

static int dvb_widgets_probe(struct platform_device *pdev)
{
    const char *str = NULL;
    int dw_num = 0;
    bool ant_power_overload_one_ping;
    int error = -EINVAL;

    pr_dbg("%s\n",__FUNCTION__);

    //判断节点是否存在
    if (!pdev->dev.of_node) {
        pr_dbg("dvb_widgets pdev->dev.of_node is NULL!\n");
        error = -EINVAL;
        goto get_node_fail;
    }

    // read string
    error = of_property_read_string(pdev->dev.of_node, "dw_name", &str);
    pr_dbg("dw_name:%s\n",str);

    // read u32
    error = of_property_read_u32(pdev->dev.of_node, "dw_num", &dw_num);
    if (error) {
        pr_err("Filed to get  dw_num\n");
    }else{
        pr_dbg("dw_num:%d\n", dw_num);
    }

    // read bool 如果dts文件定义的有ant_power_overload_one_ping此属性:true
    ant_power_overload_one_ping = of_property_read_bool(pdev->dev.of_node, "ant_power_overload_one_ping");
    if (ant_power_overload_one_ping) {
        pr_dbg("ant_power_overload_one_ping : true\n");
    } else {
        pr_dbg("ant_power_overload_one_ping : false\n");
    }

    //GPIO读取
    error = of_property_read_string(pdev->dev.of_node, "loops-gpio", &str);
    if (!error) {
        pdw_dev.loops_pin =
            desc_to_gpio(of_get_named_gpiod_flags(pdev->dev.of_node,
                              "loops-gpio", 0, NULL));
        pr_dbg("%s: %s\n", "loops-gpio", str);
    } else {
        pdw_dev.loops_pin = -1;
        pr_dbg("cannot find loops-gpio \n");
    }

    error = of_property_read_string(pdev->dev.of_node, "ch34-gpio", &str);
    if (!error) {
        pdw_dev.ch3_4_pin = 
            desc_to_gpio(of_get_named_gpiod_flags(pdev->dev.of_node,
                            "ch34-gpio", 0, NULL));
        pr_dbg("%s: %s\n", "ch34-gpio", str);
    } else {
        pdw_dev.ch3_4_pin = -1;
        pr_dbg("cannot find ch3_4_gpio\n");
    }

    error = of_property_read_string(pdev->dev.of_node, "ant_power-gpio", &str);
    if (!error) {
        pdw_dev.ant_power_pin =
            desc_to_gpio(of_get_named_gpiod_flags(pdev->dev.of_node,
                            "ant_power-gpio", 0, NULL));
        pr_dbg("ant_power-gpio\n");
    } else {
        pdw_dev.ant_power_pin = -1;
        pr_dbg("cannt find ant_power-gpio\n");
    }

    //申请GPIO
    gpio_request(pdw_dev.ant_power_pin, MODULE_NAME);
    gpio_request(pdw_dev.loops_pin, MODULE_NAME);
    gpio_request(pdw_dev.ch3_4_pin, MODULE_NAME);

    //设置GPIO 上拉状态
    gpio_set_pullup(pdw_dev.ant_power_pin, 1);
    gpio_set_pullup(pdw_dev.loops_pin, 1);
    gpio_set_pullup(pdw_dev.ch3_4_pin, 1);

    //设置GPIO输出电平
    gpio_direction_output(pdw_dev.ant_power_pin, 1); //输出高电平
    gpio_direction_output(pdw_dev.loops_pin, 0); //输出低电平
    gpio_direction_output(pdw_dev.ch3_4_pin, 0); //输出低电平

    return 0;

get_node_fail:  
    return error;
}

#ifdef CONFIG_OF 
static const struct of_device_id dvb_widgets_match[] = {
    { .compatible  = "amlogic, dvb_widgets"},
    {},
};
#else
#define dvb_widgets_match NULL
#endif


static struct platform_driver dvb_widgets_driver = {
    .probe = dvb_widgets_probe,
    .remove = dvb_widgets_remove,
    .shutdown = dvb_widgets_shutdown,
    .suspend = dvb_widgets_suspend,
    .resume = dvb_widgets_resume,
    .driver = { 
        .name = MODULE_NAME, //设备驱动名
        .of_match_table = dvb_widgets_match,  
    },
};

static int __init dvb_widgets_init(void)
{
    pr_dbg("%s\n",__FUNCTION__);
    return platform_driver_register(&dvb_widgets_driver);
}

static void __exit dvb_widgets_exit(void)
{
    pr_dbg("%s\n",__FUNCTION__);
    platform_driver_unregister(&dvb_widgets_driver);
}

module_init(dvb_widgets_init);
module_exit(dvb_widgets_exit);


MODULE_AUTHOR("Song YuLong");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AML DVB Widgets Driver.");
MODULE_AUTHOR("GOOD, Inc.");

