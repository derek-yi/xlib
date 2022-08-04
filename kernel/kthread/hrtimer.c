#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
 
static struct hrtimer timer;
ktime_t kt;
struct timespec oldtc;
 
static enum hrtimer_restart hrtimer_hander(struct hrtimer *timer)
{
	struct timespec tc;
	long diff;
    
	//printk("I am in hrtimer hander : %lu... \r\n",jiffies);
	getnstimeofday(&tc); //获取新的当前系统时间
	
	diff = tc.tv_nsec-oldtc.tv_nsec;
	oldtc = tc;
	printk("interval: %ld ns\r\n", diff);
    hrtimer_forward(timer,timer->base->get_time(), kt);

    return HRTIMER_RESTART;
}
 
static int __init test_init(void)
{
    printk("---------test start-----------\r\n");
    
	getnstimeofday(&oldtc);  //获取当前系统时间
    kt = ktime_set(0,1000000);//1ms
    hrtimer_init(&timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
    hrtimer_start(&timer,kt,HRTIMER_MODE_REL);
    timer.function = hrtimer_hander;
    return 0;
}
 
static void __exit test_exit(void)
{
    hrtimer_cancel(&timer);
    printk("------------test over---------------\r\n");
}
 
module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
 