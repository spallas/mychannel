#include "sempahore.h"

int sem_init(int init_value, int n){
	int id_sem = semget(IPC_PRIVATE, n, IPC_CREAT|IPC_EXCL|0666);
	if ( id_sem == -1 ) printf("Errore nella chiamata semget");

	union semun arg;
	int arr[n];
	for (size_t i = 0; i < count; i++) {
		arr[i] = init_value;
	}
	arg.array = arr;

	int ret = semctl(id_sem, 0, SETALL, arg);
	if(ret == -1) printf("Error initializing sem");
	return id_sem;
}

void sem_wait(int sem, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = -1;
	op.sem_flag = 0;
	int ret = semop(mutex, &op)
	if(ret == -1) printf("Error waiting sem");
}

void sem_post(int sem, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = 1;
	op.sem_flag = 0;
	int ret = semop(mutex, &op, 1)
	if(ret == -1) printf("Error posting sem");
}


int mutex_init(){
	int mutex_id = semget(IPC_PRIVATE, 1, IPC_CREAT|IPC_EXCL|0666);
	if( mutex_id == -1) printf("Error initializing mutex");

	int ret = semgctl(mutex_id, 0, SETVAL, 1);
	if(ret == -1) printf("Error initializing mutex");

	return mutex_id;
}

void mutex_lock(int mutex){
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flag = 0;
	int ret = semop(mutex, &op, 1)
	if(ret == -1) printf("Error locking mutex");
}

void mutex_unlock(int mutex){
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flag = 0;
	int ret = semop(mutex, &op, 1);
	if(ret == -1) printf("Errore unlocking mutex");
}

void sem_close(int sem, int index){
	union semun arg = 0;
	int ret = semclt(sem, index, IPC_RMID, arg);
	if(ret == -1) printf("Error closing sem");
}
