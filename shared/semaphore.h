#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef _SEMAPHORE_H_

union semun
{
               int              val;    /* Value for SETVAL */
               struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
               unsigned short  *array;  /* Array for GETALL, SETALL */
               struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
           };


int sem_init(int init_value, int n);
void sem_post(int semaphore, int index);
void sem_wait(int semaphore, int index);
void sem_close(int semaphore);
int mutex_init(int n);
void mutex_lock(int mutex, int index);
void mutex_unlock(int mutex, int index);

#endif
