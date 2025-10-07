#include "spinlock.h"
#include "stdint.h"
#include "stddef.h"

void spinlock_init(spinlock_t* lock) {
    lock->locked = 0;
}

void spinlock_acquire(spinlock_t* lock) {
        if ((uint64_t)lock < 0xffff800000000000 || lock == NULL) {
        extern void serial_write_string(const char*);
        serial_write_string("\n\n=== SPINLOCK DEBUG ===\n");
        serial_write_string("Invalid lock pointer detected!\n");
        serial_write_string("Lock address: 0x");
        uint64_t addr = (uint64_t)lock;
        char hex[32];
        for(int i = 15; i >= 0; i--) {
            int digit = (addr >> (i * 4)) & 0xF;
            hex[15-i] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        }
        hex[16] = '\n';
        hex[17] = '\0';
        serial_write_string(hex);
        uint64_t rbp;
        __asm__ volatile("mov %%rbp, %0" : "=r"(rbp));
        serial_write_string("RBP (stack frame): 0x");
        serial_write_string("\nHALTING\n");
        __asm__ volatile("cli; hlt");
        for(;;);
    }
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        while (lock->locked)
            __asm__ volatile("pause" ::: "memory");
    }
}

void spinlock_release(spinlock_t* lock) {
    __sync_lock_release(&lock->locked);
}