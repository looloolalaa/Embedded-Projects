#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define PIN1 6
#define PIN2 13
#define PIN3 19
#define PIN4 26

#define STEPS 8
#define ONEROUND 512
#define ONE_STEP ONEROUND/STEPS

int location = 0;

int blue[8] = {1, 1, 0, 0, 0, 0, 0, 1};
int pink[8] = {0, 1, 1, 1, 0, 0, 0, 0};
int yellow[8] = {0, 0, 0, 1, 1, 1, 0, 0};
int orange[8] = {0, 0, 0, 0, 0, 1, 1, 1};

void setstep(int p1, int p2, int p3, int p4){
	gpio_set_value(PIN1, p1);
	gpio_set_value(PIN2, p2);
	gpio_set_value(PIN3, p3);
	gpio_set_value(PIN4, p4);
}

void backward(int step, int delay){
	int i=0, j=0;
	
	for(i=0;i<ONE_STEP*step;i++){
		for(j=STEPS;j>0;j--){
			setstep(blue[j], pink[j], yellow[j], orange[j]);
			udelay(delay);
		}
	}
}

void forward(int step, int delay){
	int i=0, j=0;
	
	for(i=0;i<ONE_STEP*step;i++){
		for(j=0;j<STEPS;j++){
			setstep(blue[j], pink[j], yellow[j], orange[j]);
			udelay(delay);
		}
	}
}

void moveDegree(int degree, int delay, int direction){
	int step = degree/45;

	if(direction == 0) forward(step, delay);
	else if (direction == 1) backward(step, delay);
	else printk("ch10: wrong direction\n");

	
}

static int __init ch10_init(void){

	gpio_request_one(PIN1, GPIOF_OUT_INIT_LOW, "p1");
	gpio_request_one(PIN2, GPIOF_OUT_INIT_LOW, "p2");
	gpio_request_one(PIN3, GPIOF_OUT_INIT_LOW, "p3");
	gpio_request_one(PIN4, GPIOF_OUT_INIT_LOW, "p4");

	//setstep(0, 0, 0, 0);
	moveDegree(90, 3000, 0);
	mdelay(1200);
	moveDegree(45, 3000, 0);
	mdelay(1200);
	moveDegree(45, 3000, 0);
	mdelay(1200);
	moveDegree(180, 3000, 0);
	//setstep(0, 0, 0, 0);
	return 0;
}

static void __exit ch10_exit(void){
	setstep(0, 0, 0, 0);
	gpio_free(PIN1);
	gpio_free(PIN2);
	gpio_free(PIN3);
	gpio_free(PIN4);
}

module_init(ch10_init);
module_exit(ch10_exit);

