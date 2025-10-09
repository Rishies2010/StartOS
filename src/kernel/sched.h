#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include "../cpu/isr.h"

#define TASK_STACK_SIZE 8192
#define TIME_SLICE 10

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD
} task_state_t;

typedef struct task {
    uint64_t pid;
    char name[64];
    task_state_t state;
    
    // Saved context (registers)
    registers_t regs;
    
    // Stack
    uint64_t kernel_stack;
    uint64_t stack_size;
    
    // Scheduling
    uint64_t time_slice_remaining;
    
    // Linked list
    struct task* next;
} task_t;

// Initialize the scheduler
void sched_init(void);

// Start the scheduler (call after all init is done)
void sched_start(void);

// Create a new task
task_t* task_create(void (*entry)(void), const char* name);

// Yield CPU to next task
void sched_yield(void);

// Called by timer interrupt
void sched_tick(void);

// Get current running task
task_t* sched_current_task(void);

// Perform actual context switch (implemented in assembly)
extern void task_switch(registers_t* old_regs, registers_t* new_regs);

#endif