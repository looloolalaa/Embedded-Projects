obj-m:= ku_sa.o

KDIR:=~/linux-rpi/
ARM:= ARCH=arm CROSS_COMPILE=/usr/bin/arm-linux-gnueabi-
PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules
	arm-linux-gnueabi-gcc app_sensor.c -o app_sensor
	arm-linux-gnueabi-gcc app_act.c -o app_act
	arm-linux-gnueabi-gcc app_stop_sensor.c -o app_stop_sensor
	arm-linux-gnueabi-gcc app_stop_act.c -o app_stop_act
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) clean
	rm $(obj-m:.o=_app)
scp:
	scp $(obj-m:.o=.ko) mknod.sh app_sensor app_act app_stop_sensor app_stop_act pi@10.10.10.12:~/

