#include "core.h"
#include "../../libk/debug/log.h"
#include "../../drv/vga.h"

// There are a minimum 2 CPUs. BSP is id 0.

void ap_main(void)
{
    __asm__ __volatile__("sti");
    for (;;)
        __asm__ __volatile__("hlt");
}