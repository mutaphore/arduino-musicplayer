#ifndef SYNCHRO_H
#define SYNCHRO_H

#include "os.h"

volatile typedef struct {
   int owner;
   uint8_t list[MAX_THREADS];    //List of threads waiting
   uint8_t count;                //Number of threads waiting
   int front;                    //Head index of the waiting list
   int end;                      //End index of the waiting list
} mutex_t;

volatile typedef struct {
   int value;
   uint8_t list[MAX_THREADS];    //List of threads waiting
   int front;                    //Head index of the waiting list
   int end;                      //End index of the waiting list
} semaphore_t;

//Synchronization functions
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
void sem_init(semaphore_t *s, int8_t value);
void sem_wait(semaphore_t *s);
void sem_signal(semaphore_t *s);
void sem_signal_swap(semaphore_t *s);
void yield();

#endif
