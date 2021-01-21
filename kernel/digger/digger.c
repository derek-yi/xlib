
#include <linux/init.h>             
#include <linux/module.h>           
#include <linux/kernel.h>
#include <linux/kthread.h>      
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>

typedef int (* cmd_cb)(char *in_cmd, char *out_buf);

typedef struct 
{
    char    *cmd_key;
    char    *help_str;
    cmd_cb   cb_func;
    void     *cookie;
}DIG_CMD_S;

int kernel_info(char *in_cmd, char *out_buf)
{
    int offset = 0;
    
    offset += sprintf(out_buf + offset, "jiffies %lu \n", jiffies);
    //offset += sprintf(out_buf + offset, "jiffies %lu \n", jiffies);

    return 0;
}

DIG_CMD_S cmd_list[128] = 
{
    { "kernel",     "get kernel info",      kernel_info,        NULL},
    { "tasklet",    "raise tasklet",        kernel_info,        NULL},
    { NULL,         NULL,   NULL,    NULL}
};

char cmd_buff[256];

char out_string[1024];

static ssize_t result_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%s", out_string);
}

static ssize_t help_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    int i;
    int offset = 0;

    memset(out_string, 0, sizeof(out_string));
    for (i = 0; i < sizeof(cmd_list)/sizeof(DIG_CMD_S); i++) {
        if (cmd_list[i].cb_func != NULL) {
            offset += sprintf(out_string + offset, "%-16s -- %s \n", cmd_list[i].cmd_key, cmd_list[i].help_str);
        }
    }

    return sprintf(buf, "%s", out_string);
}

static ssize_t digger_cmd_show(struct kobject* kobjs, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "digger_cmd: %s", cmd_buff);
}

static ssize_t digger_cmd_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret = -1;
    int i;

    snprintf(cmd_buff, sizeof(cmd_buff), "%s", buf);
    memset(out_string, 0, sizeof(out_string));
    for (i = 0; i < sizeof(cmd_list)/sizeof(DIG_CMD_S); i++) {
        if ( (cmd_list[i].cb_func != NULL) 
             && ( !memcmp(cmd_buff, cmd_list[i].cmd_key, strlen(cmd_list[i].cmd_key)) ) ) {
            ret = cmd_list[i].cb_func(cmd_buff, out_string);
            break;
        }
    }
    
    if (ret != 0) {
        sprintf(out_string, "failed: %s", cmd_buff);
    }
    
    return count;
}

static struct kobj_attribute digger_run_attr = __ATTR_RO(result);
static struct kobj_attribute digger_help_attr = __ATTR_RO(help);
static struct kobj_attribute digger_cmd_attr = __ATTR(input, 0660, digger_cmd_show, digger_cmd_store);  

static struct attribute *digger_attrs[] = {
    &digger_run_attr.attr,
    &digger_help_attr.attr,
    &digger_cmd_attr.attr,
    NULL,
};

static struct attribute_group digger_attr_grp = {
    .name = "digger",
    .attrs = digger_attrs,
};

static struct kobject *my_kobj;

static int __init sysfs_ctrl_init(void)
{
    printk(KERN_INFO "sysfs_ctrl_init\n");
    my_kobj = kobject_create_and_add("derek", kernel_kobj->parent);
    sysfs_create_group(my_kobj, &digger_attr_grp);
    return 0;
}

static void __exit sysfs_ctrl_exit(void)
{
    printk(KERN_INFO "sysfs_ctrl_exit\n");
    sysfs_remove_group(my_kobj, &digger_attr_grp);
    kobject_put(my_kobj);
}

/*
sudo rmmod digger.ko
sudo insmod digger.ko

sudo chmod 666 /sys/derek/digger/help
sudo chmod 666 /sys/derek/digger/input
sudo chmod 666 /sys/derek/digger/result

echo kernel > /sys/derek/digger/input
echo on > /sys/derek/digger/input

cat /sys/derek/digger/input
cat /sys/derek/digger/result
*/

module_init(sysfs_ctrl_init);
module_exit(sysfs_ctrl_exit);

MODULE_AUTHOR("derek.yi");  
MODULE_LICENSE("GPL");  

