#include "sched.h"
#include "../drv/vga.h"
#include <stddef.h>
#include "../libk/debug/log.h"

#define MAX_TASKS 10
static task_t tasks[MAX_TASKS];
static int current_task = 0;
spinlock_t schedlock;
int schedulerun = 1;

// Multitasking using cooperative multitasking scheduler.

int add_task(task_func_t func, const char *name)
{
    spinlock_acquire(&schedlock);
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (!tasks[i].active)
        {
            tasks[i].func = func;
            tasks[i].name = name;
            tasks[i].active = 1;
            log("[SCHED] New Task Added : %s.", 1, 0, name);
            spinlock_release(&schedlock);
            return i;
        }
    }
    spinlock_release(&schedlock);
    return -1;
}

void remove_task(task_func_t func)
{
    spinlock_acquire(&schedlock);
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i].active && tasks[i].func == func)
        {
            tasks[i].active = 0;
            log("[SCHED] Task Removed: %s.", 1, 0, tasks[i].name);
            tasks[i].name = NULL;
            spinlock_release(&schedlock);
            return;
        }
    }
    spinlock_release(&schedlock);
}

void schedule(void)
{
    while (schedulerun)
    {
        spinlock_acquire(&schedlock);
        int has_active_tasks = 0;
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (tasks[i].active)
            {
                has_active_tasks = 1;
                break;
            }
        }
        if (!has_active_tasks)
        {
            spinlock_release(&schedlock);
            return;
        }
        if (tasks[current_task].active)
        {
            tasks[current_task].func();
        }
        current_task = (current_task + 1) % MAX_TASKS;
        spinlock_release(&schedlock);
    }
}