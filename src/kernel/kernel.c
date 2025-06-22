/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/ports.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/rtc.h"

void pit_handler(){
    send_eoi(IRQ0);
}

// void shutdown(){
//     outportw(0x604, 0x2000);
//     outportw(0xB004, 0x2000);
//     outportw(0x4004, 0x3400);
//     outportb(0xB2, 0x0F);
//     outportw(0x8900, 0x2001);
//     outportw(0x8900, 0xFF00);
//     __asm__ __volatile__("cli");
//     __asm__ __volatile__("hlt");
// }

void _start(void){
    serial_init();
    init_kernel_heap();
    init_gdt();
    init_idt();
    remap_pic();
    init_pit(10);
    rtc_initialize();
    asm volatile("sti");
    register_interrupt_handler(IRQ0, pit_handler, "PIT Handler");
    log("Get stick bugged lol", 3);
    for(;;);
}