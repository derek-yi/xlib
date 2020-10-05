


/**
 * @file   foo_drv.c
 * @author Late Lee <latelee@163.com>
 * @date   Wed Jun  7 22:21:19 2019
 * 
 * @brief  ����dtsʾ��
 * 
 * @note   ��ȡdts��ֵ��ѧϰdts�������в��־��棬��Ӱ��
 */

#include <linux/module.h>
#include <linux/kernel.h>       /**< printk() */
#include <linux/init.h>

#include <linux/types.h>        /**< size_t */
#include <linux/errno.h>        /**< error codes */
#include <linux/string.h>

#include <linux/of.h>
#include <linux/of_device.h>

static int foo_remove(struct platform_device *dev)
{
    printk(KERN_NOTICE "remove...\n");

    return 0;
}

static int foo_probe(struct platform_device *dev)
{
    int ret = 0;
    struct device_node* np = dev->dev.of_node;
    struct device_node* child = NULL;
    const char* str = NULL;
    bool enable = false;
    u8 value = 0;
    u16 value16 = 0;
    u32 value32 = 0;
    
    // ����dts��ȡAPI
    if(np == NULL)
    {
        pr_info("of_node is NULL\n");
        return 0;
    }
    
    of_property_read_string(np, "status", &str); // ���ַ���

    enable = of_property_read_bool(np, "enable"); // bool���ͣ����ж�ĳ�ֶδ��ڲ�����
    of_property_read_u32(np, "myvalue", &value32); // һ��أ���ʹ��u32��ȡ��ֵ
    of_property_read_u8(np, "value", &value);
    of_property_read_u16(np, "value16", &value16);
    
    u32 data[3] = {0};
	u32 tag = 0;
    // a-cell��һ�����飬Ĭ�϶���1����
    of_property_read_u32(np, "a-cell", &tag);
    // Ҳ���Զ�ȡָ����С�����飨��һ����ȫ���ģ�
    of_property_read_u32_array(np, "a-cell", data, ARRAY_SIZE(data));
    
    printk("of read status: %s enable: %d value: %d %d %d\n", str, enable, value, value16, value32);
    printk("of read tag: %d data: %d %d %d\n", tag, data[0], data[1], data[2]);
    
    // ��ȡ�ӽڵ����
    int count = of_get_available_child_count(np);
    
    // ���������ӽڵ㣬����ʽ��ȡ����
    int index = 0;
    for_each_available_child_of_node(np,child)
    {
        const char* label = of_get_property(child,"label",NULL) ? : child->name;
        const char* note = of_get_property(child,"note",NULL) ? : child->name;
        printk("of read: label: %s note: %s\n", label, note);
    }
    return ret;
}

static struct of_device_id foo_of_match[] = {
	{ .compatible = "ll,jimkent-foo", },
	{ /* sentinel */ }
};

static struct platform_driver foo_driver = {
	.driver = {
		.name = "foo",
		.of_match_table = of_match_ptr(foo_of_match),
	},
	.probe  = foo_probe,
    .remove = foo_remove,
};

static int __init foo_drv_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&foo_driver);
    if (ret)
    {
        pr_info("platform_driver_register failed!\n");
        return ret;
    }
    
    pr_info("Init OK!\n");
    
    return ret;
}

static void __exit foo_drv_exit(void)
{
    platform_driver_unregister(&foo_driver);
}

module_init(foo_drv_init);
module_exit(foo_drv_exit);

MODULE_AUTHOR("Late Lee");
MODULE_DESCRIPTION("Simple platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:foo");


#ifdef xxx
myfoo {
    compatible = "ll,jimkent-foo";
    status = "okay"; // string
    enable; // bool������ֵ
    myvalue = <250>; // Ĭ����32λ�����ʹ��8λ��ȡ�����Ϊ0
    value = /bits/ 8 <88>; // 8λ������ֵ
    value16 = /bits/ 16 <166>; // 16λ������ֵ
    a-cell = <1 2 3 4 5>; // ����
    // �ӽڵ�
    foo {
        label = "foo";
        note = "this is foo";
    };
    bar {
        label = "bar";
        note = "this is bar";
    };
};

#endif
