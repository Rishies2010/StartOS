/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"

void _start(void){
    serial_init();
    init_kernel_heap();
    log("Get stick bugged lol", 3);
    for(;;);
}