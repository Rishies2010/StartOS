#ifndef SPINLOCK_H
#define SPINLOCK_H

typedef struct {
    volatile int locked;
} spinlock_t;

void spinlock_init(spinlock_t* lock);
void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

#endif