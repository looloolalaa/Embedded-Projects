#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

#define LED 5
#define SWITCH 12

struct my_timer_info{
	struct timer_list timer;
	long delay_jiffies;
	int data;
};

static struct my_timer_info my_timer;

static void my_timer_func(struct timer_list *t){
	struct my_timer_info *info = from_timer(info, t, timer);

	if(gpio_get_value(SWITCH))
		gpio_set_value(LED, 1);
	else
		gpio_set_value(LED, 0);
	
	mod_timer(&my_timer.timer, jiffies + info->delay_jiffies);
}

static int __init ch5_init(void){
	printk("ch5: init module \n");

	gpio_request_one(LED, GPIOF_OUT_INIT_LOW, "LED");
	gpio_request_one(SWITCH, GPIOF_IN, "SWITCH");
	
	my_timer.delay_jiffies = msecs_to_jiffies(500);
	my_timer.data = 100;
	timer_setup(&my_timer.timer, my_timer_func, 0);
	my_timer.timer.expires = jiffies + my_timer.delay_jiffies;
	add_timer(&my_timer.timer);

	return 0;
}

static void __exit ch5_exit(void){
	printk("ch5: exit module \n");

	gpio_set_value(LED, 0);
	gpio_free(LED);
	gpio_free(SWITCH);
	del_timer(&my_timer.timer);
}

module_init(ch5_init);
module_exit(ch5_exit);

