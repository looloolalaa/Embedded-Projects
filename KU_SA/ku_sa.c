#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <stdarg.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#define DEV_NAME "ku_sa_dev"

#define MAX_TIMING 85
#define DHT11 21

#define PIN1 6
#define PIN2 13
#define PIN3 19
#define PIN4 26
#define LED1 4

#define SENSOR1 17

#define STEPS 8
#define ONEROUND 512

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4

#define SIMPLE_IOCTL_NUM 'z'
#define KU_SENSE_ACTIVE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define KU_ACT_ACTIVE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define KU_SENSE_STOP _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define KU_ACT_STOP _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM4, unsigned long *)

MODULE_LICENSE("GPL");

int z = 0;
int initial_z = 0;

spinlock_t my_lock;

struct my_timer_info{
	struct timer_list timer;
	long delay_jiffies;
};
static struct my_timer_info sensor_timer;
static struct my_timer_info act_timer;
static int dht11_data[5] = {0, };

struct factor{
	int humidity;
	int temperature;
	bool touch;
};
static struct factor input = {0, 0, false};

int blue[8] = {1, 1, 0, 0, 0, 0, 0, 1};
int pink[8] = {0, 1, 1, 1, 0, 0, 0, 0};
int yellow[8] = {0, 0, 0, 1, 1, 1, 0, 0};
int orange[8] = {0, 0, 0, 0, 0, 1, 1, 1};

static int irq_num;

static irqreturn_t simple_sensor_isr(int irq, void* dev_id){
	unsigned long flags;
	local_irq_save(flags);
	printk("ku_sa: Detect touch\n");
	input.touch = true;
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

void setstep(int p1, int p2, int p3, int p4){
	gpio_set_value(PIN1, p1);
	gpio_set_value(PIN2, p2);
	gpio_set_value(PIN3, p3);
	gpio_set_value(PIN4, p4);
}

void forward(int round, int delay){
	int i=0, j=0;
	
	for(i=0;i<ONEROUND*round;i++){
		for(j=0;j<STEPS;j++){
			setstep(blue[j], pink[j], yellow[j], orange[j]);
			udelay(delay);
		}
	}
	setstep(0, 0, 0, 0);
}

static bool dht11_read(void){
	
	int last_state = 1;
	int counter = 0;
	int i=0, j=0;

	dht11_data[0] = dht11_data[1] = dht11_data[2] = dht11_data[3] = dht11_data [4] = 0;
	gpio_direction_output(DHT11, 0);
	gpio_set_value(DHT11, 0);
	mdelay(18);
	gpio_set_value(DHT11, 1);
	udelay(40);
	gpio_direction_input(DHT11);

	for(i=0;i<MAX_TIMING;i++){
		counter = 0;
		while(gpio_get_value(DHT11) == last_state){
			counter++;
			udelay(1);
			if(counter == 255){
				break;
			}
		}
		last_state = gpio_get_value(DHT11);

		if(counter == 255){
			break;
		}

		if( (i >= 4) && (i % 2 == 0) ){
			dht11_data[j/8] <<= 1;
			if(counter > 16){
				dht11_data[j/8] |= 1;
			}
			j++;
		}
	}

	if( (j >= 40) &&
			(dht11_data[4] == ((dht11_data[0]+dht11_data[1]+dht11_data[2]+dht11_data[3]) & 0xFF))){
		if(dht11_data[0] == 0 || dht11_data[2] == 0){
			return false;
		}
		return true;
	}else{
		return false;
	}
}


struct factor calculate_humi_temp(void){
	struct factor box;
	int humi = dht11_data[1];
	int temp = dht11_data[3];
	//while(humi>=1)
	//	humi/=10.0;
	//while(temp>=1)
	//	temp/=10.0;
	humi = dht11_data[0];
	temp = dht11_data[2];

	box.humidity = humi;
	box.temperature = temp;
	return box;
	
}

static int calculate_z(void){

	int new_z=0;
	int humi = input.humidity;
	int temp = input.temperature;
	//new_z = (9/5)*temp-(55/100)*(1-humi)*((9/5)*temp-26)+32;
	
	int T = 9*temp/5;
	
	new_z = T - (55)*(100 - humi)*(T - 26)/10000 + 32;
	if(input.touch)
		new_z += 1;
	return new_z;
}


//sensor
static void sensor_timer_func(struct timer_list *t){
	struct my_timer_info *info = from_timer(info, t, timer);
	int new_z = 0;

	//온습도 읽기
	if(dht11_read()){
		input = calculate_humi_temp();
	}	
	

	new_z = calculate_z();

	spin_lock(&my_lock);
	z = new_z;
	spin_unlock(&my_lock);

	mod_timer(&sensor_timer.timer, jiffies + info->delay_jiffies);
}

//actuator
static void act_timer_func(struct timer_list *t){
	struct my_timer_info *info = from_timer(info, t, timer);
	struct factor first_input, second_input;
	int delay = 0;
	int new_z = 0;
	int read_limit_time = jiffies + msecs_to_jiffies(10000);
	int now_time;
	
	printk("ku_sa: --------------------");

	//온습도 읽기
	
	while(1){
		now_time = jiffies;
		if(read_limit_time<now_time){
			printk("ku_sa: problem in reading humi and temp \n");
			del_timer(&act_timer.timer);
			break;
		}
		if(dht11_read()){
			first_input = calculate_humi_temp();
		}
		if(dht11_read()){
			second_input = calculate_humi_temp();
		}

		if(first_input.humidity == 0 || first_input.temperature == 0 || second_input.humidity == 0 || second_input.temperature == 0)
			continue;
		else if(first_input.humidity == second_input.humidity && first_input.temperature == second_input.temperature){
			input = second_input;
			break;
		}
	}
	printk("ku_sa: [Clean Data] Humidity: %d Temperature = %d C\n", second_input.humidity, second_input.temperature);
	new_z = calculate_z();


	spin_lock(&my_lock);
	z = new_z;
	spin_unlock(&my_lock);

	printk("ku_sa: z = %d", z);
	/*if(z<72)
		delay = 4000;
	else if(72<=z && z<74)
		delay = 3500;
	else if(74<=z && z<76)
		delay = 3000;
	else if(76<=z && z<78)
		delay = 2500;
	else if(78<=z && z<80)
		delay = 2000;
	else
		delay = 1500;*/

	if(z < initial_z + 3){
		delay = 3300;
		gpio_set_value(LED1, 0);
	}
	else{
		delay = 1800;
		gpio_set_value(LED1, 1);
	}
	
	input.touch = false;
	printk("ku_sa: spinnig ... with delay [%d]", delay);
	forward(1, delay);
	printk("ku_sa: spin done");

	mod_timer(&act_timer.timer, jiffies + info->delay_jiffies);
}

//sensor 타이머 실행
void active_sensor(void){
	printk("ku_sa: --active sensor func--");

	add_timer(&sensor_timer.timer);
}

//actuator 타이머 실행
void active_act(void){	
	printk("ku_sa: --active act func--");

	add_timer(&act_timer.timer);
}


//sensor 타이머 중지
void stop_sensor(void){
	printk("ku_sa: --stop sensor func--");
	del_timer(&sensor_timer.timer);
	
}

//actuator 타이머 중지
void stop_act(void){
	printk("ku_sa: --stop act func--");
	del_timer(&act_timer.timer);
}

static long ku_sa_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	int ret = 0;

	switch (cmd) {
	case KU_SENSE_ACTIVE:
		active_sensor();
		break;
	case KU_ACT_ACTIVE:
		active_act();
		break;
	case KU_SENSE_STOP:
		stop_sensor();
		break;
	case KU_ACT_STOP:
		stop_act();
		break;
	default:
		printk("ku_sa: wrong cmd\n");
		ret = -1;
		break;
	}

	return ret;
}

static int ku_sa_open(struct inode *inode, struct file *file) {
	return 0;
}

static int ku_sa_release(struct inode *inode, struct file *file) {
	return 0;
}

struct file_operations ku_sa_fops = {
	.unlocked_ioctl = ku_sa_ioctl,
	.open = ku_sa_open,
	.release = ku_sa_release
};

static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init ku_sa_init(void) {
	int ret;
	struct factor first_input, second_input;

	sensor_timer.delay_jiffies = msecs_to_jiffies(1000);
	timer_setup(&sensor_timer.timer, sensor_timer_func, 0);
	sensor_timer.timer.expires = jiffies + sensor_timer.delay_jiffies;


	act_timer.delay_jiffies = msecs_to_jiffies(1000);
	timer_setup(&act_timer.timer, act_timer_func, 0);
	act_timer.timer.expires = jiffies + act_timer.delay_jiffies;

	gpio_request(DHT11, "DHT11");
	gpio_request_one(PIN1, GPIOF_OUT_INIT_LOW, "p1");
	gpio_request_one(PIN2, GPIOF_OUT_INIT_LOW, "p2");
	gpio_request_one(PIN3, GPIOF_OUT_INIT_LOW, "p3");
	gpio_request_one(PIN4, GPIOF_OUT_INIT_LOW, "p4");
	gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");

	gpio_request_one(SENSOR1, GPIOF_IN, "sensor1");
	irq_num = gpio_to_irq(SENSOR1);
	ret = request_irq(irq_num, simple_sensor_isr, IRQF_TRIGGER_RISING, "sensor_irq", NULL);
	if(ret){
		printk("simple_sensor: Unable to request IRQ:%d\n", irq_num);
		free_irq(irq_num, NULL);
	}
	else{
		disable_irq(irq_num);
	}
	enable_irq(irq_num);

	printk("ku_sa: Init Module\n");
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &ku_sa_fops);
	ret = cdev_add(cd_cdev, dev_num, 1);
	if (ret < 0) {
		printk("ku_sa: fail to add character device \n");
		return -1;
	}

	while(1){
		if(dht11_read()){
			first_input = calculate_humi_temp();
		}
		if(dht11_read()){
			second_input = calculate_humi_temp();
		}

		if(first_input.humidity == 0 || first_input.temperature == 0 || second_input.humidity == 0 || second_input.temperature == 0)
			continue;
		else if(first_input.humidity == second_input.humidity && first_input.temperature == second_input.temperature){
			input = second_input;
			break;
		}
	}
	

	z = calculate_z();
	initial_z = z;
	printk("ku_sa: Initial Humidity = %d and Temperature = %d C\n", input.humidity, input.temperature);	
	printk("ku_sa: Initial z = %d\n", initial_z);
	printk("ku_sa: Init Done\n");

	return 0;
}

static void __exit ku_sa_exit(void) {

	disable_irq(irq_num);
	free_irq(irq_num, NULL);
	gpio_free(SENSOR1);

	del_timer(&sensor_timer.timer);
	del_timer(&act_timer.timer);

	gpio_set_value(DHT11, 0);
	gpio_free(DHT11);
	gpio_free(PIN1);
	gpio_free(PIN2);
	gpio_free(PIN3);
	gpio_free(PIN4);
	gpio_free(LED1);

	printk("ku_sa: Exit Module\n");
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);

	
}

module_init(ku_sa_init);
module_exit(ku_sa_exit);
