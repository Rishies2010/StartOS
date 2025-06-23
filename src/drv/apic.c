#include "apic.h"
#include "../libk/core/mem.h"
#include "../libk/debug/log.h"
#include "../cpu/isr.h"
#include <stdint.h>
#include <stdbool.h>

static uint64_t apic_base = 0;
static bool apic_available = false;

static inline uint32_t apic_read(uint32_t reg) {
    return *((volatile uint32_t*)(apic_base + reg));
}

static inline void apic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)(apic_base + reg)) = value;
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile ("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(subleaf));
}

static bool check_apic_support(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return (edx & (1 << 9)) != 0;
}

static void apic_timer_handler() {
    apic_send_eoi();
}

bool init_apic(void) {
    if (!check_apic_support()) {
        return false;
    }
    uint64_t msr = rdmsr(APIC_BASE_MSR);
    apic_base = msr & 0xFFFFF000;
    if (!(msr & APIC_BASE_ENABLE)) {
        msr |= APIC_BASE_ENABLE;
        wrmsr(APIC_BASE_MSR, msr);}
    apic_available = true;
    apic_enable();
    register_interrupt_handler(IRQ0, apic_timer_handler, "APIC Timer");
    return true;
}

void apic_enable(void) {
    if (!apic_available) return;
    
    uint32_t spurious = apic_read(APIC_SPURIOUS);
    spurious |= APIC_SPURIOUS_ENABLE;
    spurious |= 0xFF;
    apic_write(APIC_SPURIOUS, spurious);
}

void apic_disable(void) {
    if (!apic_available) return;
    
    uint32_t spurious = apic_read(APIC_SPURIOUS);
    spurious &= ~APIC_SPURIOUS_ENABLE;
    apic_write(APIC_SPURIOUS, spurious);
}

bool apic_is_enabled(void) {
    if (!apic_available) return false;
    return (apic_read(APIC_SPURIOUS) & APIC_SPURIOUS_ENABLE) != 0;
}

uint32_t apic_get_id(void) {
    if (!apic_available) return 0;
    return (apic_read(APIC_ID) >> 24) & 0xFF;
}

void apic_send_eoi(void) {
    if (!apic_available) return;
    apic_write(APIC_EOI, 0);
}

void apic_send_ipi(uint32_t dest, uint32_t flags, uint32_t vector) {
    if (!apic_available) return;
    
    apic_write(APIC_ICRH, (dest & 0xFF) << 24);
    apic_write(APIC_ICRL, flags | (vector & 0xFF));
    
    while (apic_read(APIC_ICRL) & APIC_ICR_DELIVERY_PENDING);
}

void apic_send_init_ipi(uint32_t dest) {
    apic_send_ipi(dest, APIC_ICR_DELIVERY_INIT | APIC_ICR_DEST_PHYSICAL | APIC_ICR_LEVEL_ASSERT, 0);
}

void apic_send_startup_ipi(uint32_t dest, uint32_t vector) {
    apic_send_ipi(dest, APIC_ICR_DELIVERY_STARTUP | APIC_ICR_DEST_PHYSICAL, vector);
}

void apic_timer_init(uint32_t frequency) {
    if (!apic_available) return;
    
    apic_write(APIC_TIMER_DCR, APIC_TIMER_DIV_16);
    apic_write(APIC_LVT_TIMER, IRQ0 | APIC_LVT_TIMER_PERIODIC);
    
    uint32_t ticks_per_10ms = 0xFFFFFFFF;
    apic_write(APIC_TIMER_ICR, ticks_per_10ms);
    
    __asm__ volatile ("sti");
    
    for (volatile int i = 0; i < 10000000; i++);
    
    __asm__ volatile ("cli");
    
    uint32_t ticks_elapsed = ticks_per_10ms - apic_read(APIC_TIMER_CCR);
    uint32_t ticks_per_second = ticks_elapsed * 100;
    uint32_t ticks_per_ms = ticks_per_second / 1000;
    
    uint32_t target_ticks = ticks_per_ms * (1000 / frequency);
    apic_write(APIC_TIMER_ICR, target_ticks);
    log("APIC initialized successfully at frequency %u.", 4, 0, frequency);
}

void apic_timer_stop(void) {
    if (!apic_available) return;
    apic_write(APIC_LVT_TIMER, APIC_LVT_MASKED);
    apic_write(APIC_TIMER_ICR, 0);
}

uint32_t apic_timer_get_ticks(void) {
    if (!apic_available) return 0;
    return apic_read(APIC_TIMER_CCR);
}

void apic_mask_timer(void) {
    if (!apic_available) return;
    uint32_t lvt = apic_read(APIC_LVT_TIMER);
    lvt |= APIC_LVT_MASKED;
    apic_write(APIC_LVT_TIMER, lvt);
}

void apic_unmask_timer(void) {
    if (!apic_available) return;
    uint32_t lvt = apic_read(APIC_LVT_TIMER);
    lvt &= ~APIC_LVT_MASKED;
    apic_write(APIC_LVT_TIMER, lvt);
}