#include "semaphore.h"

int sem_init(int init_value, int n){
	int id_sem = semget(IPC_PRIVATE, n, IPC_CREAT|IPC_EXCL|0666);
	if ( id_sem == -1 ) LOGe("Error in semget at sem_init()");

	union semun arg;
	unsigned short arr[n];
	for (size_t i = 0; i < n; i++) {
		arr[i] = init_value;
	}
	arg.array = arr;

	int ret = semctl(id_sem, 0, SETALL, arg);
	if(ret == -1) LOGe("Error initializing sem in sem_init()");
	return id_sem;
}

void sem_wait(int sem, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = -1;
	op.sem_flg = 0;
	int ret = semop(sem, &op, 1);
	if(ret == -1) LOGe("Error waiting sem");
}

void sem_post(int sem, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = 1;
	op.sem_flg = 0;
	int ret = semop(sem, &op, 1);
	if(ret == -1) LOGe("Error posting sem");
}


int mutex_init(int n){
	int mutex_id = semget(IPC_PRIVATE, n, IPC_CREAT|IPC_EXCL|0666);
	if( mutex_id == -1) LOGe("Error initializing mutex");

	union semun arg;
	unsigned short arr[n];
	for (size_t i = 0; i < n; i++) {
		arr[i] = 1;
	}
	arg.array = arr;

	int ret = semctl(mutex_id, 0, SETALL, arg);
	if(ret == -1) LOGe("Error initializing mutex");

	return mutex_id;
}

void mutex_lock(int mutex, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = -1;
	op.sem_flg = 0;
	int ret = semop(mutex, &op, 1);
	if(ret == -1) LOGe("Error locking mutex");
}

void mutex_unlock(int mutex, int index){
	struct sembuf op;
	op.sem_num = index;
	op.sem_op = 1;
	op.sem_flg = 0;
	int ret = semop(mutex, &op, 1);
	if(ret == -1) LOGe("Errore unlocking mutex");
}

void sem_close(int sem){
	int index = 0;
	union semun arg = {0};
	int ret = semctl(sem, index, IPC_RMID, arg);
	if(ret == -1) {
		char info[64];
		sprintf(info, "Error closing sem: %s, %d, %d\n",
						 strerror(errno), sem, index);
		LOGe(info);
	}
}
