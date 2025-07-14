/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/ports.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/sse.h"
#include "../cpu/isr.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/rtc.h"
#include "../drv/vga.h"
#include "../drv/apic.h"
#include "../drv/disk/ff.h"
#include "../drv/disk/ata_pio_drv.h"

void pit_handler(){
    send_eoi(IRQ0);
}

void _start(void){
    serial_init();
    init_pmm();
    init_kernel_heap();
    init_gdt();
    init_idt();
    enable_sse();
    if(init_apic()) {
        apic_timer_init(100);
    } else {
        log("APIC not available, falling back to PIT", 3, 1);
        remap_pic();
        init_pit(10);
        register_interrupt_handler(IRQ0, pit_handler, "PIT Handler");}
    rtc_initialize();
    vga_init();
    ata_init();
    asm volatile("sti");
    prints("Hello Everyone ! FONT Test here ! \nDamn, - \t good font indeed HUH ?\ni liek it\nqwertyuiopasdfghjklzxcvbnm\nQWERTYUIOPASDFGHJKLZXCVBNM");
    for(;;);
}
