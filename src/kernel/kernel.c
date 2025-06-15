/**
 * The StartOS Kernel
 */
 
#include "../drv/vga.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/keyboard.h"
#include "../libk/ports.h"
#include "../libk/mem.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"

extern uint32_t endmem;
void shutdown(void){
    terminal_setcolor(vga_entry_color(15, 0));
    vga_hide_cursor();
    clr();
    outportw(0x604, 0x2000);
    outportw(0xB004, 0x2000);
    outportw(0x4004, 0x3400);
    outportb(0xB2, 0x0F);
    outportw(0x8900, 0x2001);
    outportw(0x8900, 0xFF00);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}

void pit_handler(){
    send_eoi(IRQ0);
}

void kmain(void* multiboot_info){
    init_gdt();
    remap_pic();
    init_idt();
    init_kernel_heap((uint32_t)&endmem, 0x100000);
    init_pit(10);
    initterm();
    keyboard_initialize();
    boot();
    wprint("Development Mode Environment.\n");
    prints("Welcome to StartOS !\n");
    register_interrupt_handler(IRQ0, pit_handler);
    asm volatile("sti");
    for(;;)asm volatile("hlt");
}
