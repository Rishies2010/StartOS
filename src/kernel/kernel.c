/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "gfx.h"
#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/ports.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/sse_fpu.h"
#include "../cpu/isr.h"
#include "../libk/spinlock.h"
#include "../cpu/smp.h"
#include "../cpu/id/cpuid.h"
#include "../drv/apic_timer.h"
#include "../drv/rtc.h"
#include "../drv/vga.h"
#include "../drv/mouse.h"
#include "../drv/local_apic.h"
#include "../drv/ioapic.h"
#include "../cpu/acpi/acpi.h"
#include "../drv/speaker.h"
#include "../drv/keyboard.h"
#include "../drv/disk/ata.h"

int cursor_old_x = 0;
int cursor_old_y = 0;
int outline = 0x202020;
int fill = 0x808080;

void draw_cursor() {
    int size = 20;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size - y; x++) {
            put_pixel(cursor_old_x + x, cursor_old_y + y, 0x000000);
        }
    }
    
    int x = mouse_x();
    int y = mouse_y();
    uint8_t buttons = mouse_button();
    int current_outline = outline;
    int current_fill = fill;
    
    if (buttons & 0x01) current_fill = 0xFFFFFF;
    if (buttons & 0x02) {
        outportw(0x604, 0x2000);
        outportw(0xB004, 0x2000);
        outportw(0x4004, 0x3400);
        outportb(0xB2, 0x0F);
        outportw(0x8900, 0x2001);
        outportw(0x8900, 0xFF00);
        __asm__ __volatile__("cli");
        __asm__ __volatile__("hlt");
    }
    if (buttons & 0x04) current_fill = 0xFFFFFF;
    
    for (int py = 0; py < size; py++) {
        for (int px = 0; px < size - py; px++) {
            if (px == 0 || px == size - py - 1 || py == 0) {
                put_pixel(x + px, y + py, current_outline);
            } else {
                put_pixel(x + px, y + py, current_fill);
            }
        }
    }
    cursor_old_x = x;
    cursor_old_y = y;
}

void test_mouse(void) {
    cursor_old_x = framebuffer_width / 2;
    cursor_old_y = framebuffer_height / 2;
    while (1) {
        if (mouse_moved()) {
            draw_cursor();
        }
    }
}

void play_bootup_sequence() {
    speaker_note(3, 0);  // C4
    for(volatile int i = 0; i < 8000000; i++);
    speaker_note(3, 2);  // E4
    for(volatile int i = 0; i < 8000000; i++);
    speaker_note(3, 4);  // G4
    for(volatile int i = 0; i < 8000000; i++);
    speaker_note(4, 0);  // C5
    for(volatile int i = 0; i < 16000000; i++);
    speaker_note(4, 7);  // B5
    for(volatile int i = 0; i < 8000000; i++);
    speaker_note(5, 0);  // C6
    for(volatile int i = 0; i < 32000000; i++);
    speaker_pause();
}

void _start(void){
    serial_init();
    init_pmm();
    init_kernel_heap();
    init_vmm();
    vga_init();
    init_gdt();
    init_idt();
    AcpiInit();
    LocalApicInit();
    IoApicInit();
    init_apic_timer(10);
    enable_sse_and_fpu();
    detect_cpu_info(0);
    rtc_initialize();
    spinlock_init(&loglock);
    init_smp();
    ata_init();
    init_keyboard();
    mouse_init();
    IoApicSetEntry(g_ioApicAddr, 0, 0x20);
    IoApicSetEntry(g_ioApicAddr, 1, 0x21);
    #if debug
        log("[KERNEL] Running In Debug Mode.", 2, 1);
    #else
        aa_rect(4, 4, 180, 40, 7, makecolor(0, 0, 0), makecolor(255 ,255, 255));
        prints("\n  Welcome To StartOS !\n\n");
        draw_startlogo(framebuffer_width - 186, -16);
        play_bootup_sequence();
    #endif
    asm volatile("sti");
    test_mouse();
    for(;;);
}
