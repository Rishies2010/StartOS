#include "apic_timer.h"
#include "local_apic.h"
#include "../cpu/isr.h"
#include "../libk/debug/log.h"

#define APIC_TIMER_VECTOR 0x20
#define APIC_TIMER_LVT    0x320
#define APIC_TIMER_INIT   0x380
#define APIC_TIMER_CURRENT 0x390
#define APIC_TIMER_DIVIDE  0x3E0

static volatile uint32_t timer_ticks = 0;

void apic_timer_handler() {
    timer_ticks++;
}

void init_apic_timer(uint32_t frequency) {
    volatile uint32_t *apic = (volatile uint32_t*)g_localApicAddr;
    
    apic[APIC_TIMER_DIVIDE / 4] = 0x3;
    
    apic[APIC_TIMER_LVT / 4] = APIC_TIMER_VECTOR | (1 << 17);
    
    uint32_t ticks_per_ms = get_apic_timer_frequency() / 1000;
    uint32_t initial_count = ticks_per_ms * (1000 / frequency);
    
    apic[APIC_TIMER_INIT / 4] = initial_count;
    
    register_interrupt_handler(APIC_TIMER_VECTOR, apic_timer_handler, "APIC Timer");
    
    log("[APIC] APIC Timer initialized at %uHz.", 4, 0, frequency);
}

uint32_t get_apic_timer_frequency(void) {
    volatile uint32_t *apic = (volatile uint32_t*)g_localApicAddr;
    
    apic[APIC_TIMER_DIVIDE / 4] = 0x3;
    apic[APIC_TIMER_LVT / 4] = 0x10000;
    apic[APIC_TIMER_INIT / 4] = 0xFFFFFFFF;
    
    uint32_t start = apic[APIC_TIMER_CURRENT / 4];
    
    for(volatile int i = 0; i < 10000000; i++);
    
    uint32_t end = apic[APIC_TIMER_CURRENT / 4];
    
    return (start - end) * 16;
}

uint32_t get_timer_ticks(void) {
    return timer_ticks;
}

void sleep_ms(uint32_t ms) {
    uint32_t start = timer_ticks;
    while((timer_ticks - start) < ms);
}