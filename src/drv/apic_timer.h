#ifndef APIC_TIMER_H
#define APIC_TIMER_H

#include "stdint.h"

void init_apic_timer(uint32_t frequency);
uint32_t get_apic_timer_frequency(void);
uint32_t get_timer_ticks(void);
void sleep(uint32_t ms);
void apic_timer_handler();

#endif