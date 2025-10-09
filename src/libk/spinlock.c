#include "spinlock.h"
#include "stdint.h"
#include "stddef.h"
#include "debug/serial.h"

void spinlock_init(spinlock_t *lock)
{
    lock->locked = 0;
}

void spinlock_acquire(spinlock_t *lock)
{

    if ((uint64_t)lock < 0xffff800000000000 || lock == NULL)
    {
        serial_write_string("\n[src/kernel/spinlock.c:??]- [CPU?]-\n");
        serial_write_string("Invalid lock pointer: 0x");
        uint64_t addr = (uint64_t)lock;
        for (int i = 15; i >= 0; i--)
        {
            int digit = (addr >> (i * 4)) & 0xF;
            char c = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            serial_write_string((char[]){c, 0});
        }
        serial_write_string("\nSystem halted.\n");
        __asm__ volatile("cli; hlt");
        for (;;)
            ;
    }
    while (__sync_lock_test_and_set(&lock->locked, 1))
    {
        while (lock->locked)
        {
            __asm__ volatile("pause" ::: "memory");
        }
    }
}

void spinlock_release(spinlock_t *lock)
{
    __sync_lock_release(&lock->locked);
}