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
#include "../cpu/sse_fpu.h"
#include "../cpu/isr.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/rtc.h"
#include "../drv/vga.h"
#include "../drv/apic.h"
#include "../drv/sound.h"
#include "../drv/speaker.h"
#include "../drv/disk/ata.h"

void pit_handler(){
    send_eoi(IRQ0);
}

void _start(void){
    serial_init();
    init_pmm();
    init_kernel_heap();
    vga_init();
    init_gdt();
    init_idt();
    asm volatile("sti");
    enable_sse_and_fpu();
    if(init_apic()) {
        apic_timer_init(100);
    } else {
        log("APIC not available, falling back to PIT", 3, 1);
        remap_pic();
        init_pit(10);
        register_interrupt_handler(IRQ0, pit_handler, "PIT Handler");}
    rtc_initialize();
    ata_init();
    sound_init();
    print_mem_info(0);
    prints("\n Welcome To StartOS !\n\n");
    for(;;);
}
