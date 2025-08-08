#include "apic_timer.h"
#include "local_apic.h"
#include "../cpu/isr.h"
#include "rtc.h"
#include "../libk/debug/log.h"

#define APIC_TIMER_VEC 0x20
#define APIC_LVT_TIMER 0x320
#define APIC_TIMER_INIT 0x380
#define APIC_TIMER_CUR 0x390
#define APIC_TIMER_DIV 0x3E0

static uint32_t apic_freq = 0;

void apic_timer_handler(registers_t* regs)
{
    LocalApicSendEOI();
}

void init_apic_timer(uint32_t freq)
{
    if (freq < 100 || freq > 2000)
    {
        log("[APIC] Inappropriate frequency range : %i.", 3, 1, freq);
        return;
    }
    else if (freq < 250 || freq > 1000)
    {
        log("[APIC] Frequency outside optimal range : %i.", 2, 1, freq);
    }
    volatile uint32_t *apic = (volatile uint32_t *)g_localApicAddr;

    apic[0xF0 / 4] |= 0x100;
    register_interrupt_handler(APIC_TIMER_VEC, apic_timer_handler, "APIC Timer");

    if (!apic_freq)
    {
        apic_freq = get_apic_timer_frequency();
        if (!apic_freq)
        {
            log("[APIC] Unable to initialize timer.", 3, 1);
            return;
        };
    }

    if (freq * 16 > apic_freq)
    {
        log("[APIC] Frequency value too high. Aborting.", 3, 1);
        return;
    }
    uint32_t ticks_per_int = apic_freq / (16 * freq);

    apic[APIC_LVT_TIMER / 4] = APIC_TIMER_VEC | 0x20000;
    apic[APIC_TIMER_DIV / 4] = 0x3;
    apic[APIC_TIMER_INIT / 4] = ticks_per_int;
    log("[APIC] Timer initialized at %uHz.", 4, 0, freq);
}

uint32_t get_apic_timer_frequency()
{
    volatile uint32_t *apic = (volatile uint32_t *)g_localApicAddr;

    uint32_t old_lvt = apic[APIC_LVT_TIMER / 4];
    uint32_t old_div = apic[APIC_TIMER_DIV / 4];

    apic[APIC_LVT_TIMER / 4] = 0x10000;
    apic[APIC_TIMER_DIV / 4] = 0x3;
    apic[APIC_TIMER_INIT / 4] = 0xFFFFFFFF;

    uint32_t start = apic[APIC_TIMER_CUR / 4];
    sleep(10);
    uint32_t end = apic[APIC_TIMER_CUR / 4];

    apic[APIC_LVT_TIMER / 4] = old_lvt;
    apic[APIC_TIMER_DIV / 4] = old_div;

    uint32_t delta = start - end;
    return delta * (1000 / 10) * 16; // Hz
}
