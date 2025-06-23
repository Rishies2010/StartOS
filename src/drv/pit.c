#include "pit.h"
#include "../libk/ports.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"

#define PIT_NATURAL_FREQ 1193180

#define PIT_DATA0 0x40
#define PIT_DATA1 0x41
#define PIT_DATA2 0x42
#define PIT_COMMAND 0x43


void init_pit(uint32_t frequency)
{
    uint32_t divisor;
    if(frequency)
        divisor = PIT_NATURAL_FREQ / frequency;
    else
        divisor = 0;
    outportb(PIT_COMMAND, 0x36);
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);
    outportb(PIT_DATA0, low);
    outportb(PIT_DATA0, high);
    log("PIT Driver Initialized at %uHz.", 4, 0, frequency);
}