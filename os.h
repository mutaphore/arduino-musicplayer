#ifndef OS_H
#define OS_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define MAX_THREADS 4

//This structure defines the register order pushed to the stack on a
//system context switch.
typedef struct {
   uint8_t padding; //stack pointer is pointing to 1 byte below the top of the stack

   //Registers that will be managed by the context switch function
   uint8_t r29;
   uint8_t r28;
   uint8_t r17;
   uint8_t r16;
   uint8_t r15;
   uint8_t r14;
   uint8_t r13;
   uint8_t r12;
   uint8_t r11;
   uint8_t r10;
   uint8_t r9;
   uint8_t r8;
   uint8_t r7;
   uint8_t r6;
   uint8_t r5;
   uint8_t r4;
   uint8_t r3;
   uint8_t r2;
   uint8_t pch;
   uint8_t pcl;
} regs_context_switch;

//This structure defines how registers are pushed to the stack when
//the system tick interrupt occurs.  This struct is never directly
//used, but instead be sure to account for the size of this struct
//when allocating initial stack space
typedef struct {
   uint8_t padding; //stack pointer is pointing to 1 byte below the top of the stack

   //Registers that are pushed to the stack during an interrupt service routine
   uint8_t r31;
   uint8_t r30;
   uint8_t r27;
   uint8_t r26;
   uint8_t r25;
   uint8_t r24;
   uint8_t r23;
   uint8_t r22;
   uint8_t r21;
   uint8_t r20;
   uint8_t r19;
   uint8_t r18;
   uint8_t sreg; //status register
   uint8_t r0;
   uint8_t r1;
   uint8_t pch;
   uint8_t pcl;
} regs_interrupt;

//Thread states
volatile typedef enum {
   THREAD_RUNNING,
   THREAD_READY,
   THREAD_SLEEPING,
   THREAD_WAITING
} TState;

volatile typedef struct {
   uint8_t id;          //Thread id
   uint8_t *stackBase;  //Lowest address of stack
   uint8_t *stackEnd;   //Highest address of stack
   uint16_t tp;         //Thread stack pointer
   uint16_t userSize;   //User defined stack size
   uint32_t totSize;    //Total number of bytes allocated for stack
   uint16_t pc;         //Starting PC of thread function
   TState state;           //Thread state
   uint16_t sleep;         //Sleep ticks
   uint16_t sched_count;   //Number of times per second thread was run
   uint8_t intr_pcl;       //Interrupted PC address low byte
   uint8_t intr_pch;       //Interrupted PC address high byte
} thread_t;

volatile typedef struct {
   uint32_t runtime;                //OS up time
   uint16_t intrSec;                //Number of interrupts per second
   thread_t threads[MAX_THREADS];   //Max 8 threads
   uint8_t numThreads;              //Number of threads
   uint8_t curId;                   //Current running thread id
   uint32_t numIntr;                //Number of interrupts since OS start
} system_t;

//OS functions
void os_init();
void create_thread(uint16_t address, void* args, uint16_t stack_size);
void os_start(void);
uint8_t get_next_thread(void);
void thread_sleep(uint16_t ticks);
int main();

void start_system_timer();
void start_audio_pwm();

//Global variables
volatile system_t sysInfo;              //OS information

#endif
