/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "sched.h"
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
#include "../drv/net/e1000.h"
#include "../drv/net/tcp.h"
#include "../drv/net/pci.h"
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
    ata_init();
    init_keyboard();
    mouse_init();
    IoApicSetEntry(g_ioApicAddr, 0, 0x20);
    IoApicSetEntry(g_ioApicAddr, 1, 0x21);
    init_smp();
    pci_check_all_buses();
    e1000_init();
    #if debug
        log("[KERNEL] Running In Debug Mode.", 2, 1);
    #else
    #endif
    asm volatile("sti");
    for(;;);
}
