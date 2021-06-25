#include "ku_ipc_lib.c"

int main(){

	ku_msgget(3, KU_IPC_CREAT);

	struct msgbuf msg_read;
	ku_msgrcv(3, &msg_read, 12, 10, KU_MSG_NOERROR | 0);
	printf("read: %s\n", msg_read.text);
	
	
	return 0;

}
