
/*
https://blog.csdn.net/u013420428/article/details/78115512

*/

/*********************************************************************************
 *      Copyright:  (C) 2016 Guo Wenxue<guowenxue@gmail.com>  
 *                  All rights reserved.
 *
 *       Filename:  at91_keyled.c
 *    Description:  This is a sample driver for GPIO operation with DTS linux on at91,
 *                  which willl turn led on when a button pressed.
 *                 
 *        Version:  1.0.0(2016-6-29~)
 *         Author:  Guo Wenxue <guowenxue@gmail.com>
 *      ChangeLog:  1, Release initial version on "Wed Jun 29 12:00:44 CST 2016"
 *
 *
 *    DTS Changes:
 *                 add keyleds support in arch/arm/boot/dts/at91sam9x5cm.dtsi
 *
 *                   keyleds{
 *                          compatible = "key-leds";
 *                          gpios = <&pioB 18 GPIO_ACTIVE_LOW     priv->pin_key=of_get_gpio(pdev->dev.of_node, 0);
 *                                   &pioB 16 GPIO_ACTIVE_LOW>;   priv->pin_key=of_get_gpio(pdev->dev.of_node, 1);
 *                          status = "okay";
 *                   }
 *
 *                   1wire_cm {
 *                      ... ...
 *                      ... ...
 *                   }
 *                 
 ********************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

typedef struct keyled_priv_s 
{
    int       pin_key;  
    int       pin_led; 
    int       led_status;
} keyled_priv_t;  /*---  end of struct keyled_priv_s  ---*/


static const struct of_device_id of_key_leds_match[] = {
        { .compatible = "key-leds", },
        {},
};

MODULE_DEVICE_TABLE(of, of_key_leds_match);


static irqreturn_t key_detect_interrupt(int irq, void *dev_id)
{
    keyled_priv_t    *priv = (keyled_priv_t *)dev_id;

    priv->led_status ^= 1;
    gpio_set_value(priv->pin_led, priv->led_status);

    return IRQ_HANDLED;
}


static int at91_keyled_probe(struct platform_device *pdev)
{
    int              res;
    keyled_priv_t    *priv;

    printk(KERN_INFO "at91_keyled driver probe\n");

    if( 2 != of_gpio_count(pdev->dev.of_node) )
    {
        printk(KERN_ERR "keyled pins definition in dts invalid\n");
        return -EINVAL;
    }

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if(!priv)
        return -ENOMEM;

    platform_set_drvdata(pdev, priv);

    priv->pin_key = of_get_gpio(pdev->dev.of_node, 0);
    priv->pin_led = of_get_gpio(pdev->dev.of_node, 1);

    if( gpio_is_valid(priv->pin_key) )
    {
        if( (res=devm_gpio_request(&pdev->dev, priv->pin_key, "keyled_key")) < 0 )
        {
            dev_err(&pdev->dev, "can't request key gpio %d\n", priv->pin_key);
            return res;
        }
        dev_info(&pdev->dev, "request key gpio %d ok\n", priv->pin_key);

        if( (res=gpio_direction_input(priv->pin_key)) < 0 )
        {
            dev_err(&pdev->dev, "can't request input direction key gpio %d\n", priv->pin_key);
            return res;
        }
        dev_info(&pdev->dev, "request input direction key gpio %d ok\n", priv->pin_key);

        printk(KERN_INFO "Key gpio current status: %d\n", gpio_get_value(priv->pin_key));

        res = request_irq( gpio_to_irq(priv->pin_key), key_detect_interrupt, IRQF_TRIGGER_FALLING, "keyled", priv);
        if( res )
        {
            dev_err(&pdev->dev, "can't request IRQ<%d> for key gpio %d\n", gpio_to_irq(priv->pin_key), priv->pin_key);
            return -EBUSY;
        }
        dev_info(&pdev->dev, "request IRQ<%d> for key gpio %d ok\n", gpio_to_irq(priv->pin_key), priv->pin_key);
    }

    if( gpio_is_valid(priv->pin_led) )
    {
        if( (res = devm_gpio_request(&pdev->dev, priv->pin_led, "keyled_led")) < 0 )
        {
            dev_err(&pdev->dev, "can't request key gpio %d\n", priv->pin_led);
            return res;
        }

        if( (res = gpio_direction_output(priv->pin_led, 0)) < 0 )
        {
            dev_err(&pdev->dev, "can't request output direction key gpio %d\n", priv->pin_led);
            return res;
        }
    }

    return 0;
}

static int at91_keyled_remove(struct platform_device *pdev)
{
    keyled_priv_t    *priv = platform_get_drvdata(pdev);

    printk(KERN_INFO "at91_keyled driver remove\n");

    devm_gpio_free(&pdev->dev, priv->pin_led);
    devm_gpio_free(&pdev->dev, priv->pin_key);

    free_irq(gpio_to_irq(priv->pin_key), priv);

    devm_kfree(&pdev->dev, priv);

    return 0;
}

static struct platform_driver at91_keyled_driver = {
    .probe      = at91_keyled_probe,
    .remove     = at91_keyled_remove,
    .driver     = {
        .name   = "key-leds",
        .of_match_table = of_key_leds_match,
    },
};

module_platform_driver(at91_keyled_driver);

MODULE_AUTHOR("guowenxue <guowenxue@gmail.com>");
MODULE_DESCRIPTION("AT91 Linux DTS GPIO driver for Key and LED");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:key-leds");

