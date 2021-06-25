#include "ku_ipc_lib.c"

int main(){

	ku_msgget(3, KU_IPC_CREAT);
	
	struct msgbuf msg_write = {10, "[10] type message!"};
	ku_msgsnd(3, &msg_write, strlen(msg_write.text), KU_IPC_NOWAIT);

	msg_write.type = 11;
	strcpy(msg_write.text, "[11] type message!");
	ku_msgsnd(3, &msg_write, strlen(msg_write.text), KU_IPC_NOWAIT);

	msg_write.type = 12;
	strcpy(msg_write.text, "[12] type message!");
	ku_msgsnd(3, &msg_write, strlen(msg_write.text), KU_IPC_NOWAIT);

	msg_write.type = 13;
	strcpy(msg_write.text, "[13] type message!");
	ku_msgsnd(3, &msg_write, strlen(msg_write.text), 0);

	struct msgbuf msg_read;
	ku_msgrcv(3, &msg_read, 50, 10, KU_IPC_NOWAIT);
	printf("read: %s\n", msg_read.text);

	ku_msgclose(3);
	
	return 0;

}
