#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEV_NAME "ku_sa_dev"

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

int ku_sense_active() {

	int fd = open("/dev/ku_sa_dev", O_RDWR);
	int ret = ioctl(fd, KU_SENSE_ACTIVE, 0);
	close(fd);

	if(ret != -1)
		 printf("success to activate sensor\n");
	else
		printf("fail to activate sensor\n");

	return ret;
}

int ku_sense_stop() {

	int fd = open("/dev/ku_sa_dev", O_RDWR);
	int ret = ioctl(fd, KU_SENSE_STOP, 0);
	close(fd);

	if(ret != -1)
		 printf("success to stop sensor\n");
	else
		printf("fail to stop sensor\n");

	return ret;
}


