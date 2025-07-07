#include <stdint.h>
#include "sse.h"
#include "../libk/debug/log.h"

void enable_sse(void) {

    uint64_t cr0, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);  // Clear EM bit
    cr0 |= (1 << 1);   // Set MP bit
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);   // Set OSFXSR
    cr4 |= (1 << 10);  // Set OSXMMEXCPT
    asm volatile("mov %0, %%cr4" :: "r"(cr4));
    
    log("SSE instructions enabled.", 4, 0);
}