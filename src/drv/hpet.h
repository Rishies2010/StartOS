#ifndef HPET_H
#define HPET_H

#include <stdint.h>
#include "../cpu/isr.h"

extern volatile uint64_t hpet_ticks;

void hpet_init(void);

void hpet_irq_handler(registers_t *regs);

#endif
