#include "synchro.h"
#include "globals.h"

void sem_enqueue(semaphore_t *s, uint8_t id) {

   if (!(s->front == 0 && s->end == MAX_THREADS - 1) &&
    s->end + 1 != s->front) {

      if (s->end == -1)
         s->front = s->end = 0;
      else
         s->end = (s->end + 1) % MAX_THREADS;

      s->list[s->end] = id;
   }
}

uint8_t sem_dequeue(semaphore_t *s) {
   uint8_t id = 0;

   if (s->front != -1) {
      id = s->list[s->front];

      if (s->front == s->end)
         s->front = s->end = -1;
      else
         s->front = (s->front + 1) % MAX_THREADS;
   }
   return id;
}

void mutex_enqueue(mutex_t *m, uint8_t id) {

   if (!(m->front == 0 && m->end == MAX_THREADS - 1) &&
    m->end + 1 != m->front) {

      if (m->end == -1)
         m->front = m->end = 0;
      else
         m->end = (m->end + 1) % MAX_THREADS;

      m->list[m->end] = id;
   }
}

uint8_t mutex_dequeue(mutex_t *m) {
   uint8_t id = 0;

   if (m->front != -1) {
      id = m->list[m->front];

      if (m->front == m->end)
         m->front = m->end = -1;
      else
         m->front = (m->front + 1) % MAX_THREADS;
   }
   return id;
}


void mutex_init(mutex_t *m) {

   m->owner = -1;
   memset(m->list, 0, MAX_THREADS);
   m->count = 0;
   m->front = -1;
   m->end = -1;
}

void mutex_lock(mutex_t *m) {
   cli();

   if (m->owner == -1) {
      m->owner = sysInfo.curId;
   }
   else if (m->owner != sysInfo.curId) {
      mutex_enqueue(m, sysInfo.curId);
      m->count++;
      yield();
   }

   sei();
}

void mutex_unlock(mutex_t *m) {
   cli();
   uint8_t oldId = sysInfo.curId;
   regs_context_switch *intr;

   //Only owner can unlock
   if (m->owner == sysInfo.curId) {

      //If someone is waiting for it set that person to be the owner
      if (m->count > 0) {
         m->owner = mutex_dequeue(m);
         sysInfo.threads[m->owner].state = THREAD_READY;
         m->count--;
      }
      else
         m->owner = -1;   //Otherwise no owner
   }
   sei();
}

void sem_init(semaphore_t *s, int8_t value) {

   s->value = value;
   memset(s->list, 0, MAX_THREADS);
   s->front = -1;
   s->end = -1;
}

void sem_wait(semaphore_t *s) {
   cli();

   s->value--;
   if (s->value < 0) {
      sem_enqueue(s, sysInfo.curId);
      yield();
   }

   sei();
}

//Set first waiting thread to ready
void sem_signal(semaphore_t *s) {
   cli();
   uint8_t id;

   s->value++;
   if (s->value <= 0) {
      //Remove next thread in waitlist
      id = sem_dequeue(s);
      sysInfo.threads[id].state = THREAD_READY;
   }
   sei();
}

//Switch to first waiting thread without waiting for next interrupt
void sem_signal_swap(semaphore_t *s) {
   cli();
   uint8_t oldId = sysInfo.curId;
   regs_context_switch *intr;

   s->value++;
   if (s->value <= 0) {
      intr = (regs_context_switch *)(sysInfo.threads[oldId].tp);
      sysInfo.threads[oldId].intr_pcl = intr->pcl;
      sysInfo.threads[oldId].intr_pch = intr->pch;
      sysInfo.threads[oldId].state = THREAD_READY;

      sysInfo.curId = sem_dequeue(s);
      sysInfo.threads[sysInfo.curId].state = THREAD_RUNNING;
      sysInfo.threads[sysInfo.curId].sched_count++;
      context_switch(&sysInfo.threads[sysInfo.curId].tp,
       &sysInfo.threads[oldId].tp);
   }
   sei();
}

void yield() {
   uint8_t oldId = sysInfo.curId;
   regs_context_switch *intr;

   intr = (regs_context_switch *)(sysInfo.threads[oldId].tp);
   sysInfo.threads[oldId].intr_pcl = intr->pcl;
   sysInfo.threads[oldId].intr_pch = intr->pch;
   sysInfo.threads[oldId].state = THREAD_WAITING;

   sysInfo.curId = get_next_thread();
   sysInfo.threads[sysInfo.curId].state = THREAD_RUNNING;
   sysInfo.threads[sysInfo.curId].sched_count++;
   context_switch(&sysInfo.threads[sysInfo.curId].tp,
    &sysInfo.threads[oldId].tp);
}
