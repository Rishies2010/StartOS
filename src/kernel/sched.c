#include "sched.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"
#include "../libk/spinlock.h"

static task_t *task_list_head = NULL;
static task_t *current_task = NULL;
static uint64_t next_pid = 0;
static spinlock_t sched_lock = {0};
static volatile int scheduler_enabled = 0;

void task_exit(void)
{
    spinlock_acquire(&sched_lock);
    current_task->state = TASK_DEAD;
    spinlock_release(&sched_lock);

    log("Task %s exited.", 2, 0, current_task->name);
    sched_yield();

    log("Scheduler returned.", 0, 0);
}

static void task_entry_wrapper(void)
{
    void (*entry)(void) = (void (*)(void))current_task->regs.rbx;
    entry();
    task_exit();
}

void sched_init(void)
{
    spinlock_init(&sched_lock);
    task_list_head = NULL;
    current_task = NULL;
    scheduler_enabled = 0;
    log("Scheduler initialized (no idle task)", 4, 0);
}

task_t *task_create(void (*entry)(void), const char *name)
{
    spinlock_acquire(&sched_lock);

    task_t *task = (task_t *)kmalloc(sizeof(task_t));
    if (!task)
    {
        spinlock_release(&sched_lock);
        return NULL;
    }

    memset(task, 0, sizeof(task_t));

    task->pid = next_pid++;
    strncpy(task->name, name, 63);
    task->name[63] = '\0';
    task->state = TASK_READY;
    task->time_slice_remaining = TIME_SLICE;
    task->stack_size = TASK_STACK_SIZE;

    task->kernel_stack = (uint64_t)kmalloc(TASK_STACK_SIZE);
    if (!task->kernel_stack)
    {
        kfree(task);
        spinlock_release(&sched_lock);
        return NULL;
    }

    memset((void *)task->kernel_stack, 0, TASK_STACK_SIZE);

    memset(&task->regs, 0, sizeof(registers_t));

    uint64_t stack_top = task->kernel_stack + TASK_STACK_SIZE;
    stack_top &= ~0xF;

    task->regs.rip = (uint64_t)task_entry_wrapper;
    task->regs.rbx = (uint64_t)entry;
    task->regs.cs = 0x08;
    task->regs.ss = 0x10;
    task->regs.ds = 0x10;
    task->regs.userrsp = stack_top;
    task->regs.rbp = stack_top;
    task->regs.rflags = 0x202;

    if (!task_list_head)
    {
        task_list_head = task;
        task->next = task;
        task->state = TASK_RUNNING;
        current_task = task;
    }
    else if (task_list_head->next == task_list_head)
    {
        task_list_head->next = task;
        task->next = task_list_head;
    }
    else
    {
        task_t *old_next = task_list_head->next;
        task_list_head->next = task;
        task->next = old_next;
    }

    spinlock_release(&sched_lock);

    log("Created task: %s (PID %d)", 1, 0, name, task->pid);
    return task;
}

void sched_start(void)
{
    scheduler_enabled = 1;
    if (current_task)
        return;
    current_task = task_list_head;
    if(current_task)
        current_task->state = TASK_RUNNING;
    task_switch(NULL, &current_task->regs);
}

static task_t *get_next_task(void)
{
    if (!current_task || !current_task->next)
        return NULL;

    task_t *start = current_task->next;
    task_t *iter = start;

    do
    {
        if (iter->state == TASK_READY || iter->state == TASK_RUNNING)
            return iter;
        iter = iter->next;
    } while (iter != start);

    return current_task;
}

void sched_yield(void)
{
    if (!scheduler_enabled)
        return;

    spinlock_acquire(&sched_lock);

    if (!current_task)
    {
        spinlock_release(&sched_lock);
        return;
    }

    task_t *old_task = current_task;
    task_t *new_task = get_next_task();

    if (new_task == old_task)
    {
        spinlock_release(&sched_lock);
        return;
    }

    if (old_task->state == TASK_RUNNING)
        old_task->state = TASK_READY;

    old_task->time_slice_remaining = TIME_SLICE;

    new_task->state = TASK_RUNNING;
    new_task->time_slice_remaining = TIME_SLICE;
    current_task = new_task;

    spinlock_release(&sched_lock);

    task_switch(&old_task->regs, &new_task->regs);
}

void sched_tick(void)
{
    if (!scheduler_enabled || !current_task)
        return;

    spinlock_acquire(&sched_lock);

    if (current_task->time_slice_remaining > 0)
        current_task->time_slice_remaining--;

    if (current_task->time_slice_remaining == 0)
    {
        spinlock_release(&sched_lock);
        sched_yield();
        return;
    }

    spinlock_release(&sched_lock);
}

task_t *sched_current_task(void)
{
    return current_task;
}
