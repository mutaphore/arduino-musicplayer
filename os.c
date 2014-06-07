#include "globals.h"
#include "os.h"

//Get the the next "Ready" thread - Round robin
uint8_t get_next_thread(void) {
   uint8_t id;

   for (id = 1; id < sysInfo.numThreads; id++)
      if (sysInfo.threads[id].state == THREAD_READY)
         return id;

   return 0;
}

//This interrupt routine is automatically run every 10 milliseconds
ISR(TIMER0_COMPA_vect) {
   volatile uint8_t oldId = sysInfo.curId, i;
   volatile regs_interrupt *intr;

   sysInfo.numIntr++;

   //Save interrupted PC (4 locals, 1 pad byte, 2 arguments)
   intr = (regs_interrupt *)(sysInfo.threads[oldId].tp +
    sizeof(regs_context_switch) + 4 - 1 + 2);
   sysInfo.threads[oldId].intr_pcl = intr->pcl;
   sysInfo.threads[oldId].intr_pch = intr->pch;

   //The following statement tells GCC that it can use registers r18-r27,
   //and r30-31 for this interrupt routine.  These registers (along with
   //r0 and r1) will automatically be pushed and popped by this interrupt routine.
   asm volatile ("" : : : "r18", "r19", "r20", "r21", "r22", "r23", "r24", \
                 "r25", "r26", "r27", "r30", "r31");

   //Decrement sleep timer and check for any expired ones
   for (i = 0; i < sysInfo.numThreads; i++) {
      if (sysInfo.threads[i].state == THREAD_SLEEPING) {
         if (--sysInfo.threads[i].sleep == 0)
            sysInfo.threads[i].state = THREAD_READY;
      }
   }

   //Get the thread id of the next thread to run
   sysInfo.threads[oldId].state = THREAD_READY;
   sysInfo.curId = get_next_thread();
   sysInfo.threads[sysInfo.curId].state = THREAD_RUNNING;
   sysInfo.threads[sysInfo.curId].sched_count++;

   context_switch(&sysInfo.threads[sysInfo.curId].tp,
    &sysInfo.threads[oldId].tp);
}

//This interrupt routine is run once a second
//The 2 interrupt routines will not interrupt each other
ISR(TIMER1_COMPA_vect) {

   sysInfo.runtime++;
}

//new_tp: r25:24, old_tp: r23:r22
__attribute__((naked)) void context_switch(uint16_t* new_tp, uint16_t* old_tp) {

   //Save registers in regs_context_switch to stack
   asm volatile ("PUSH r2\n"
                 "PUSH r3\n"
                 "PUSH r4\n"
                 "PUSH r5\n"
                 "PUSH r6\n"
                 "PUSH r7\n"
                 "PUSH r8\n"
                 "PUSH r9\n"
                 "PUSH r10\n"
                 "PUSH r11\n"
                 "PUSH r12\n"
                 "PUSH r13\n"
                 "PUSH r14\n"
                 "PUSH r15\n"
                 "PUSH r16\n"
                 "PUSH r17\n"
                 "PUSH r28\n"
                 "PUSH r29");

   //Store old SP address to old_tp
   asm volatile ("CLR r31\n");
   asm volatile ("LDI r30, 0x5D\n");
   asm volatile ("LD r16, Z\n");
   asm volatile ("CLR r31\n");
   asm volatile ("LDI r30, 0x5E\n");
   asm volatile ("LD r17, Z\n");
   asm volatile ("MOV r30, r22\n");
   asm volatile ("MOV r31, r23\n");
   asm volatile ("ST Z+, r16\n");   //Low byte
   asm volatile ("ST Z, r17");      //High byte

   //Load new SP address from new_tp
   asm volatile ("MOV r30, r24\n");
   asm volatile ("MOV r31, r25\n");
   asm volatile ("LD r16, Z+\n");   //Low byte
   asm volatile ("LD r17, Z\n");    //High byte
   asm volatile ("CLR r31\n");
   asm volatile ("LDI r30, 0x5D\n");
   asm volatile ("ST Z, r16\n");
   asm volatile ("CLR r31\n");
   asm volatile ("LDI r30, 0x5E");
   asm volatile ("ST Z, r17\n");

   //Load registers from stack
   asm volatile ("POP r29\n"
                 "POP r28\n"
                 "POP r17\n"
                 "POP r16\n"
                 "POP r15\n"
                 "POP r14\n"
                 "POP r13\n"
                 "POP r12\n"
                 "POP r11\n"
                 "POP r10\n"
                 "POP r9\n"
                 "POP r8\n"
                 "POP r7\n"
                 "POP r6\n"
                 "POP r5\n"
                 "POP r4\n"
                 "POP r3\n"
                 "POP r2");

   //return PC
   asm volatile ("RET");

}

__attribute__((naked)) void thread_start(void) {
   //enable interrupts
   sei();

   //Load args from r5:r4 to r25:r24
   asm volatile ("MOV r24, r4");
   asm volatile ("MOV r25, r5");

   //Use r3:r2 for function address
   asm volatile ("MOV r30, r2");   //Copy function address to Z
   asm volatile ("MOV r31, r3");
   asm volatile ("IJMP");           //Jump to function
}

void os_start(void) {

   start_system_timer();
   clear_screen();

   //Setup main idle thread #0
   sysInfo.threads[0].id = 0;
   sysInfo.threads[0].stackBase = 0x8FF;
   sysInfo.threads[0].stackEnd = 0x0;
   sysInfo.threads[0].userSize = 0x0;
   sysInfo.threads[0].pc = main;
   sysInfo.threads[0].state = THREAD_RUNNING;
   sysInfo.threads[0].sleep = 0;
   sysInfo.threads[0].sched_count = 0;
   sysInfo.threads[0].intr_pcl = 0;
   sysInfo.threads[0].intr_pch = 0;
   sysInfo.threads[0].totSize = 0x7FF;

   context_switch(&sysInfo.threads[0].tp, &sysInfo.threads[0].tp);
   
}

void os_init() {

   sysInfo.runtime = 0;
   sysInfo.intrSec = 0;
   sysInfo.numThreads = 1;
   sysInfo.numIntr = 0;
   sysInfo.curId = 0;        //Start with main thread 0 (idle)
}

void create_thread(uint16_t address, void *args, uint16_t stack_size) {
   uint8_t id;

   id = sysInfo.numThreads++;   //Increment numThreads;
   sysInfo.threads[id].totSize =
    stack_size + sizeof(regs_context_switch) + sizeof(regs_interrupt) + 32;

   //Set thread info
   sysInfo.threads[id].id = id;
   sysInfo.threads[id].stackBase = calloc(1, sysInfo.threads[id].totSize);
   sysInfo.threads[id].userSize = stack_size;
   sysInfo.threads[id].pc = address;
   sysInfo.threads[id].sleep = 0;
   sysInfo.threads[id].sched_count = 0;
   sysInfo.threads[id].intr_pcl = 0;
   sysInfo.threads[id].intr_pch = 0;
   sysInfo.threads[id].stackEnd =
    sysInfo.threads[id].stackBase + sysInfo.threads[id].totSize;

   //Put thread_start function address in PC
   *(sysInfo.threads[id].stackEnd - 1) = (uint8_t)(thread_start);
   *(sysInfo.threads[id].stackEnd - 2) =
    (uint8_t)((uint16_t)thread_start >> 8);

   //Put function address in r3:r2
   *(sysInfo.threads[id].stackEnd - 3) = (uint8_t)(address);
   *(sysInfo.threads[id].stackEnd - 4) = (uint8_t)(address >> 8);

   if (args != NULL) {
      //Put function args in r5:r4
      *(sysInfo.threads[id].stackEnd - 5) = (uint8_t)args;
      *(sysInfo.threads[id].stackEnd - 6) = (uint8_t)((uint16_t)args >> 8);
   }

   //Store stack pointer, set thread state to ready
   sysInfo.threads[id].tp =
    (uint16_t)(sysInfo.threads[id].stackEnd - sizeof(regs_context_switch));
   sysInfo.threads[id].state = THREAD_READY;
}

void thread_sleep(uint16_t ticks) {
   cli();
   uint8_t oldId = sysInfo.curId;
   regs_context_switch *intr;

   intr = (regs_context_switch *)(sysInfo.threads[oldId].tp);
   sysInfo.threads[oldId].intr_pcl = intr->pcl;
   sysInfo.threads[oldId].intr_pch = intr->pch;
   sysInfo.threads[oldId].state = THREAD_SLEEPING;
   sysInfo.threads[oldId].sleep = ticks;

   sysInfo.curId = get_next_thread();
   sysInfo.threads[sysInfo.curId].state = THREAD_RUNNING;
   context_switch(&sysInfo.threads[sysInfo.curId].tp,
    &sysInfo.threads[oldId].tp);
   sei();
}
