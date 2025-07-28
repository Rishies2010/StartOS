#ifndef SCHED_H
#define SCHED_H
#include "../libk/spinlock.h"

typedef void (*task_func_t)(void);
typedef struct
{
    task_func_t func;
    const char *name;
    int active;
} task_t;

extern spinlock_t schedlock;
extern int schedulerun;

void remove_task(task_func_t func);
void schedule(void);
int add_task(task_func_t func, const char *name);

#endif