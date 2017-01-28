#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int sem_init(int init_value, int n);
void sem_post(int semaphore, int index);
void sem_wait(int semaphore, int index);
void sem_close(int semaphore, int index);
int mutex_init();
void mutex_lock(int mutex);
void mutex_unlock(int mutex);