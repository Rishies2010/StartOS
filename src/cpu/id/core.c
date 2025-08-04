#include "core.h"

void ap_main(void)
{
    __asm__ __volatile__("sti");
    for (;;)
        __asm__ __volatile__("hlt");
}