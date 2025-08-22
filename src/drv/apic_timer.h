#ifndef APIC_TIMER_H
#define APIC_TIMER_H

#include "stdint.h"
#include "../cpu/isr.h"

void init_apic_timer(uint32_t frequency);
uint32_t get_apic_timer_frequency(void);
void sleep(uint32_t ms);
void apic_timer_handler(registers_t* regs);

#endif