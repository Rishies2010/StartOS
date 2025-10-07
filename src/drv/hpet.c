#include "../cpu/isr.h"
#include "../drv/local_apic.h"
#include "../libk/ports.h"
#include "../libk/debug/log.h"
#include "../kernel/sched.h"
#include <stdint.h>

#define HPET_BASE 0xFED00000
#define HPET_TICK_NS 1000000

volatile uint64_t hpet_ticks = 0;

typedef struct
{
    uint64_t general_cap_id;
    uint64_t reserved0;
    uint64_t general_config;
    uint64_t reserved1;
    uint64_t general_int_status;
    uint64_t reserved2;
    uint64_t main_counter;
    uint64_t reserved3[3];
} __attribute__((packed)) hpet_regs_t;

typedef struct
{
    uint64_t config;
    uint64_t comparator;
} __attribute__((packed)) hpet_timer_t;

static volatile hpet_regs_t *hpet = (volatile hpet_regs_t *)HPET_BASE;
static volatile hpet_timer_t *timer0 = (volatile hpet_timer_t *)(HPET_BASE + 0x100);

void hpet_irq_handler(registers_t *regs)
{
    (void)regs;
    hpet_ticks++;
    hpet->general_int_status = 1;
    sched_tick();
    LocalApicSendEOI();
}

void hpet_init(void)
{

    hpet->general_config &= ~1ULL;

    timer0->config = 0;

    uint64_t freq = hpet->general_cap_id >> 32;
    uint64_t ticks_per_ms = freq / 1000;
    timer0->comparator = ticks_per_ms;
    timer0->config = (1ULL << 4) |
                     (0 << 3) |
                     (1 << 2);

    hpet->general_config |= 1ULL;

    register_interrupt_handler(0x22, hpet_irq_handler, "HPET Timer");
}
