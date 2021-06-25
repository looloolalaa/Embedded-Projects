#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "ku_ipc.h"
#define DEV_NAME "ku_ipc_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4

#define SIMPLE_IOCTL_NUM 'z'
#define KU_IPC_GET _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define KU_IPC_CLOSE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define KU_IPC_SEND _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define KU_IPC_RECEIVE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM4, unsigned long *)

struct msgbuf{
	long type;
	char text[128];
};

struct param {
	int key;
	int msgflg;
	struct msgbuf* msgp;
	int msgsz;
	long msgtyp;
};

int ku_msgget(int key, int msgflg) {

	struct param args;
	args.key = key;
	args.msgflg = msgflg;

	int fd = open("/dev/ku_ipc_dev", O_RDWR);
	int ret = ioctl(fd, KU_IPC_GET, &args);
	close(fd);

	if(ret != -1)
		 printf("[%d] queue open\n", ret);
	else
		printf("[%d] queue open fail\n", ret);


	return ret;
}

int ku_msgclose(int msqid) {

	struct param args;
	args.key = msqid;

	int fd = open("/dev/ku_ipc_dev", O_RDWR);
	int ret = ioctl(fd, KU_IPC_CLOSE, &args);
	close(fd);

	if(ret == 0)
		printf("[%d] queue close\n", msqid);
	else
		printf("[%d] queue close fail\n", msqid);

	return ret;
}

int ku_msgsnd(int msqid, struct msgbuf* msgp, int msgsz, int msgflg) {

	struct param args;
	args.key = msqid;
	args.msgp = msgp;
	args.msgsz = msgsz;
	args.msgflg = msgflg;

	int fd = open("/dev/ku_ipc_dev", O_RDWR);
	int ret = ioctl(fd, KU_IPC_SEND, &args);
	close(fd);

	if(ret == 0)
		printf("send success\n");
	else
		printf("send fail\n");

	return ret;

}

int ku_msgrcv(int msqid, struct msgbuf* msgp, int msgsz, long msgtyp, int msgflg) {
	struct param args;
	args.key = msqid;
	args.msgp = msgp;
	args.msgsz = msgsz;
	args.msgtyp = msgtyp;
	args.msgflg = msgflg;

	int fd = open("/dev/ku_ipc_dev", O_RDWR);
	int ret = ioctl(fd, KU_IPC_RECEIVE, &args);
	close(fd);

	if(ret < 0)
		printf("receive fail\n");
	else
		printf("receive success & msg len [%d]\n", ret);

	return ret;
}
