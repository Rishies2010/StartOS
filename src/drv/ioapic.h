#include "stdint.h"

extern uint8_t *g_ioApicAddr;

void IoApicInit();
void IoApicSetEntry(uint8_t *base, uint8_t index, uint64_t data);
