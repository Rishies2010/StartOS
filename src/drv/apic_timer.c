#include "apic_timer.h"
#include "local_apic.h"
#include "../cpu/isr.h"
#include "../libk/debug/log.h"

#define APIC_TIMER_VEC 0x20
#define APIC_LVT_TIMER 0x320
#define APIC_TIMER_INIT 0x380
#define APIC_TIMER_CUR 0x390
#define APIC_TIMER_DIV 0x3E0

static uint32_t ticks = 0;
static uint32_t apic_freq = 0;

void apic_timer_handler() {
    LocalApicSendEOI();
    ticks++;
}

void init_apic_timer(uint32_t freq) {
    volatile uint32_t *apic = (volatile uint32_t*)g_localApicAddr;
    
    apic[0xF0 / 4] |= 0x100;
    register_interrupt_handler(APIC_TIMER_VEC, apic_timer_handler, "APIC Timer");
    
    if (!apic_freq) {
        apic_freq = get_apic_timer_frequency();
        if (!apic_freq){log("[APIC] Unable to initialize timer.", 3, 1); return;};
    }
    
    uint32_t ticks_per_int = apic_freq / (16 * freq);
    if (!ticks_per_int) ticks_per_int = 1000;
    
    apic[APIC_LVT_TIMER / 4] = APIC_TIMER_VEC | 0x20000;
    apic[APIC_TIMER_DIV / 4] = 0x3;
    apic[APIC_TIMER_INIT / 4] = ticks_per_int;
    log("[APIC] Timer initialized at %uHz.", 4, 0, freq);
}

uint32_t get_apic_timer_frequency() {
    volatile uint32_t *apic = (volatile uint32_t*)g_localApicAddr;
    
    uint32_t old_lvt = apic[APIC_LVT_TIMER / 4];
    uint32_t old_div = apic[APIC_TIMER_DIV / 4];
    
    apic[APIC_LVT_TIMER / 4] = 0x10000;
    apic[APIC_TIMER_DIV / 4] = 0x3;
    apic[APIC_TIMER_INIT / 4] = 0xFFFFFFFF;
    
    uint32_t start = apic[APIC_TIMER_CUR / 4];
    for(volatile int i = 0; i < 1000000; i++);
    uint32_t end = apic[APIC_TIMER_CUR / 4];
    
    apic[APIC_LVT_TIMER / 4] = old_lvt;
    apic[APIC_TIMER_DIV / 4] = old_div;
    
    return (start - end) * 320;
}

uint32_t get_timer_ticks() {
    return ticks;
}

void sleep_ms(uint32_t ms) {
    uint32_t start = ticks;
    while((ticks - start) < (ms / 10)) {
        __asm__ volatile("hlt");
    }
}