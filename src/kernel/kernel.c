/**
 * The StartOS Kernel
 */
 
#include "../drv/vga.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/keyboard.h"
#include "../drv/filesystem.h"
#include "../libk/ports.h"
#include "../libk/mem.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../logic/log/logging.h"

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
    init_kernel_heap((uint32_t)&endmem, 0x100000);
    init_log();
    log("Memory Initialized", 1, 0);
    init_gdt();
    remap_pic();
    init_idt();
    init_pit(10);
    initterm();
    keyboard_initialize();
    boot();
    prints("Welcome to StartOS !\n\n- Contents of logfile:\n\n");
    char buffer[FS_SIZE_MAX + 1];
    size_t bytes_read;
    if (fs_read("logfile", buffer, FS_SIZE_MAX, &bytes_read)) {
        buffer[bytes_read] = '\0';
        prints(buffer);
        prints("\n");}
    register_interrupt_handler(IRQ0, pit_handler);
    asm volatile("sti");
    for(;;)asm volatile("hlt");
}
