/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/ports.h"
#include "../libk/gfx/logo.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/sse_fpu.h"
#include "../cpu/isr.h"
#include "../drv/apic_timer.h"
#include "../drv/rtc.h"
#include "../drv/vga.h"
#include "../drv/local_apic.h"
#include "../drv/ioapic.h"
#include "../cpu/acpi/acpi.h"
#include "../drv/speaker.h"
#include "../drv/keyboard.h"
#include "../drv/disk/ata.h"

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

void draw_startos_logo(int x_offset, int y_offset) {
    for (uint32_t y = 0; y < startos.height; ++y) {
        for (uint32_t x = 0; x < startos.width; ++x) {
            int idx = (y * startos.width + x) * startos.bytes_per_pixel;

            uint8_t r = 255 - startos.pixel_data[idx + 0];
            uint8_t g = 255 - startos.pixel_data[idx + 1];
            uint8_t b = 255 - startos.pixel_data[idx + 2];
            if(!(r == 0 && g == 0 && b == 0))
            put_pixel(x + x_offset, y + y_offset, makecolor(r, g, b));
        }
    }
}

void _start(void){
    serial_init();
    init_pmm();
    init_kernel_heap();
    vga_init();
    init_gdt();
    init_idt();
    AcpiInit();
    LocalApicInit();
    IoApicInit();
    init_apic_timer(10);
    enable_sse_and_fpu();
    rtc_initialize();
    IoApicSetEntry(g_ioApicAddr, 1, 33);
    init_keyboard();
    ata_init();
    asm volatile("sti");
    prints("\n Welcome To StartOS !");
    #if debug
        prints(" (DEBUG Mode)\n\n");
    #else
        prints("\n\n");
        draw_startos_logo(-24, 5);
        play_bootup_sequence();
    #endif
    for(;;);
}
