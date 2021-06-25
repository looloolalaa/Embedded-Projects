#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define SENSOR1 17
#define LED1 5
#define DEV_NAME "ch8_dev"

static int irq_num;

struct my_timer_info{
	struct timer_list timer;
	long delay_jiffies;
};
static struct my_timer_info my_timer;

static void my_timer_func(struct timer_list *t){
	gpio_set_value(LED1, 0);
}

static int ch8_open(struct inode *inode, struct file *file){
	printk("ch8: open\n");
	enable_irq(irq_num);
	return 0;
}

static int ch8_release(struct inode *inode, struct file* file){
	printk("ch8: close \n");
	gpio_set_value(LED1, 0);
	disable_irq(irq_num);
	return 0;
}

struct file_operations ch8_fops = {
	.open = ch8_open,
	.release = ch8_release,
};

static irqreturn_t ch8_isr(int irq, void* dev_id){
	unsigned long flags;
	local_irq_save(flags);

	printk("ch8: Detect\n");
	gpio_set_value(LED1, 1);

	del_timer(&my_timer.timer);
	my_timer.delay_jiffies = msecs_to_jiffies(2000);
	timer_setup(&my_timer.timer, my_timer_func, 0);
	my_timer.timer.expires = jiffies + my_timer.delay_jiffies;
	add_timer(&my_timer.timer);

	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init ch8_init(void){
	int ret;

	printk("ch8: Init Module\n");

	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &ch8_fops);
	cdev_add(cd_cdev, dev_num, 1);

	gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");
	gpio_request_one(SENSOR1, GPIOF_IN, "sensor1");
	irq_num = gpio_to_irq(SENSOR1);
	ret = request_irq(irq_num, ch8_isr, IRQF_TRIGGER_RISING, "sensor_irq", NULL);
	if(ret){
		printk("ch8: Unable to request IRQ:%d\n", irq_num);
		free_irq(irq_num, NULL);
	}
	else{
		disable_irq(irq_num);
	}


	return 0;
}

static void __exit ch8_exit(void){
	printk("ch8: Exit Module\n");
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);

	free_irq(irq_num, NULL);
	gpio_free(SENSOR1);

	gpio_set_value(LED1, 0);
	gpio_free(LED1);
	del_timer(&my_timer.timer);
}

module_init(ch8_init);
module_exit(ch8_exit);

