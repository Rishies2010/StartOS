/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../drv/pic.h"
#include "../drv/pit.h"

void _start(void){
    serial_init();
    init_kernel_heap();
    init_gdt();
    init_idt();
    remap_pic();
    init_pit(10);
    log("Get stick bugged lol", 3);
    for(;;);
}