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
#include <asm/delay.h>
#include <stdarg.h>
#include <linux/wait.h>
#include <linux/sched.h>

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

MODULE_LICENSE("GPL");

struct msgbuf {
	long type;
	char text[128];
};

struct msg_node {
	struct list_head list;
	struct msgbuf msg;
};

struct param {
	int key;
	int msgflg;
	struct msgbuf* msgp;
	int msgsz;
	long msgtyp;
};

spinlock_t my_lock;
struct msg_node* tmp_node;
static struct msg_node msg_node_head[10];
int reference_counter[10] = { 0 };

wait_queue_head_t my_wq;

int get_queue_length(int key) {
	struct list_head* pos = 0;
	int count = 0;
	list_for_each(pos, &(msg_node_head[key].list)) {
		count++;
	}
	return count;
}

int get_queue_size(int key) {
	struct list_head* pos = 0;
	struct msg_node* node = 0;
	int size = 0;
	list_for_each(pos, &(msg_node_head[key].list)) {
		node = list_entry(pos, struct msg_node, list);
		size += strlen((node->msg).text);
	}
	return size;
}

int msgget(int key, int msgflg) {

	int ret = -1;
	printk("ku_ipc: --get func--");
	if (!(0 <= key && key <= 9))
		return -1;

	switch (msgflg) {
	case KU_IPC_CREAT:
		ret = key;
		reference_counter[key] += 1;
		break;
	case KU_IPC_EXCL:
		if (reference_counter[key] != 0)   //already in using
			ret = -1;
		else {
			ret = key;
			reference_counter[key] += 1;
		}
		break;
	default:
		printk("ku_ipc: wrong flag\n");
		break;
	}

	return ret;

}

int msgclose(int msqid) {
	int ret = -1;
	printk("ku_ipc: --close func--");
	if (reference_counter[msqid] == 0 || !(0 <= msqid && msqid <= 9))  //no using
		ret = -1;
	else {
		reference_counter[msqid] -= 1;
		ret = 0;
	}
	return ret;
}

int send(int msqid, struct msgbuf* msgp) {
	int ret;
	spin_lock(&my_lock);
	//새 노드 할당
	tmp_node = (struct msg_node*)kmalloc(sizeof(struct msg_node), GFP_KERNEL);

	//그 노드의 msg에 유저버퍼 복사
	ret = copy_from_user(&(tmp_node->msg), msgp, sizeof(struct msgbuf));

	//노드 -> 메세지 큐에 추가
	list_add_tail(&(tmp_node->list), &(msg_node_head[msqid].list));
	spin_unlock(&my_lock);
	return ret;
}

bool isFull(int msqid, int msgsz) {
	return get_queue_length(msqid) == KU_IPC_MAXMSG || (get_queue_size(msqid) + msgsz) > KU_IPC_MAXVOL;
}

bool isEmpty(int msqid) {
	return list_empty(&(msg_node_head[msqid].list));
}

int msgsnd(int msqid, struct msgbuf* msgp, int msgsz, int msgflg) {
	int ret;
	printk("ku_ipc: --send func--");

	if (reference_counter[msqid] == 0 || !(0 <= msqid && msqid <= 9))  //no using
		return -1;

	if (isFull(msqid, msgsz)) {
		//full queue
		printk("ku_ipc: [%d] queue is full", msqid);

		switch (msgflg) {
		case KU_IPC_NOWAIT:
			return -1;
		case 0:
			printk("ku_ipc: Process %i (%s) sleep\n", current->pid, current->comm);
			ret = wait_event_interruptible_exclusive(my_wq, !isFull(msqid, msgsz));
			if (ret < 0)
				return ret;

			printk("ku_ipc: Process %i (%s) wake up\n", current->pid, current->comm);
			send(msqid, msgp);
			wake_up_interruptible(&my_wq);
			return 0;
		default:
			printk("ku_ipc: wrong flag\n");
			break;
		}

		return -1;
	}
	else { //메세지 전송
		send(msqid, msgp);
		wake_up_interruptible(&my_wq);
		return 0;
	}

}

int msgrcv(int msqid, struct msgbuf* msgp, int msgsz, long msgtyp, int msgflg) {

	int len = -1;
	int ret;
	struct msg_node* node;
	struct list_head* pos = 0;
	struct list_head* q = 0;
	struct msgbuf tmp_msg;

	printk("ku_ipc: --receive func--");
	if (reference_counter[msqid] == 0 || !(0 <= msqid && msqid <= 9))  //no using
		return -1;


	if (isEmpty(msqid)) {
		//queue empty
		printk("ku_ipc: [%d] queue is Empty\n", msqid);
		if (msgflg & KU_IPC_NOWAIT) {
			return -1;
		}
		else {
			printk("ku_ipc: Process %i (%s) sleep\n", current->pid, current->comm);
			ret = wait_event_interruptible_exclusive(my_wq, !isEmpty(msqid));
			if (ret < 0)
				return ret;

			printk("ku_ipc: Process %i (%s) wake up\n", current->pid, current->comm);
			len = msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
			return len;
		}
		
	}
	else {
		printk("ku_ipc: [%d] queue is Not Empty\n", msqid);
		list_for_each_safe(pos, q, &(msg_node_head[msqid].list)) {
			//첫 번째 노드
			node = list_entry(pos, struct msg_node, list);

			if ((node->msg).type == msgtyp) { //타입이 맞는 노드

				if (strlen((node->msg).text) > msgsz) {
					//메세지 길이가 길 때
					if (msgflg & KU_MSG_NOERROR) {
						tmp_msg.type = (node->msg).type;
						strncpy(tmp_msg.text, (node->msg).text, msgsz);
						tmp_msg.text[msgsz] = '\0';

						len = msgsz;
						
						//유저버퍼에 전달
						copy_to_user(msgp, &(tmp_msg), sizeof(struct msgbuf));
						//리스트에서 노드 삭제
						list_del(pos);
						kfree(node);
						wake_up_interruptible(&my_wq);
						return len;
					}
					else {
						return -1;
					}
				}
				else {
					len = strlen((node->msg).text);

					//유저버퍼에 전달
					copy_to_user(msgp, &(node->msg), sizeof(struct msgbuf));

					//리스트에서 노드 삭제
					list_del(pos);

					kfree(node);
					wake_up_interruptible(&my_wq);

					return len;
				}
			}

		}

		printk("ku_ipc: no found msgtyp\n");

	}

	
	return len;

}

static long ku_ipc_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	int ret = 0;
	struct param* user_buf = (struct param*)vmalloc(sizeof(struct param));
	copy_from_user(user_buf, (struct param*)arg, sizeof(struct param));


	switch (cmd) {
	case KU_IPC_GET:
		ret = msgget(user_buf->key, user_buf->msgflg);
		if (ret != -1)
			printk("ku_ipc: [%d] queue open\n", ret);
		else
			printk("ku_ipc: [%d] queue open fail\n", ret);
		break;
	case KU_IPC_CLOSE:
		ret = msgclose(user_buf->key);
		if (ret == 0)
			printk("ku_ipc: [%d] queue close\n", user_buf->key);
		else
			printk("ku_ipc: [%d] queue close fail\n", user_buf->key);
		break;
	case KU_IPC_SEND:
		ret = msgsnd(user_buf->key, user_buf->msgp, user_buf->msgsz, user_buf->msgflg);
		if (ret == 0)
			printk("ku_ipc: send success [%s]\n", (user_buf->msgp)->text);
		else
			printk("ku_ipc: send fail\n");
		break;
	case KU_IPC_RECEIVE:
		ret = msgrcv(user_buf->key, user_buf->msgp, user_buf->msgsz, user_buf->msgtyp, user_buf->msgflg);
		if (ret < 0)
			printk("ku_ipc: receive fail\n");
		else
			printk("ku_ipc: receive success & msg len [%d]\n", ret);
		break;
	default:
		printk("ku_ipc: wrong cmd\n");
		ret = -1;
		break;
	}

	vfree(user_buf);
	return ret;
}

static int ku_ipc_open(struct inode *inode, struct file *file) {
	return 0;
}

static int ku_ipc_release(struct inode *inode, struct file *file) {
	return 0;
}

struct file_operations ku_ipc_fops = {
	.unlocked_ioctl = ku_ipc_ioctl,
	.open = ku_ipc_open,
	.release = ku_ipc_release
};

static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init ku_ipc_init(void) {
	int ret;

	int i = 0;
	while (i < 10) {
		reference_counter[i] = 0;
		i++;
	}

	i = 0;
	while (i < 10) {
		INIT_LIST_HEAD(&(msg_node_head[i].list)); //메세지 큐 헤드 초기화
		i++;
	}

	printk("ku_ipc: Init Module\n");
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &ku_ipc_fops);
	ret = cdev_add(cd_cdev, dev_num, 1);
	if (ret < 0) {
		printk("ku_ipc: fail to add character device \n");
		return -1;
	}

	init_waitqueue_head(&my_wq);

	return 0;
}

static void __exit ku_ipc_exit(void) {

	struct msg_node* tmp = 0;
	struct list_head* pos = 0;
	struct list_head* q = 0;

	int i = 0;
	while (i < 10) {
		list_for_each_safe(pos, q, &(msg_node_head[i].list)) {
			tmp = list_entry(pos, struct msg_node, list);
			list_del(pos);
			kfree(tmp);
		}
		i++;
	}

	printk("ku_ipc: Exit Module\n");
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(ku_ipc_init);
module_exit(ku_ipc_exit);
