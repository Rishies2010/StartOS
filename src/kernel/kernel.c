/**
 * The StartOS Kernel
 */

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
#include "../kernel/sched.h"
#include "../cpu/id/cpuid.h"
#include "../drv/rtc.h"
#include "../drv/hpet.h"
#include "../drv/vga.h"
#include "../drv/mouse.h"
#include "../drv/local_apic.h"
#include "../drv/ioapic.h"
#include "../drv/net/e1000.h"
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

void idle(void){
    for(;;)asm volatile("hlt");
}

void test_task_a(void) {
    int count = 0;
    while(1) {
        log("Task A running: %d", 1, 1, count++);
        for(volatile int i = 0; i < 50000000; i++);
    }
}

void test_task_b(void) {
    int count = 0;
    while(1) {
        log("Task B running: %d", 1, 1, count++);
        for(volatile int i = 0; i < 50000000; i++);
    }
}

void _start(void){
    serial_init();
    init_pmm();
    init_vmm();
    init_kernel_heap();
    vga_init();
    init_gdt();
    init_idt();
    AcpiInit();
    LocalApicInit();
    IoApicInit();
    sched_init();
    rtc_initialize();
    hpet_init();
    enable_sse_and_fpu();
    detect_cpu_info(0);
    ata_init();
    init_keyboard();
    mouse_init();
    init_smp();
    IoApicSetIrq(g_ioApicAddr, 8, 0x28, LocalApicGetId());  // RTC
    IoApicSetIrq(g_ioApicAddr, 2, 0x22, LocalApicGetId()); // HPET
    IoApicSetIrq(g_ioApicAddr, 0, 0x20, LocalApicGetId());  // PIT
    IoApicSetIrq(g_ioApicAddr, 1, 0x21, LocalApicGetId());  // KBD
    pci_initialize_system();
    e1000_init();
    #if debug
        log("Running In Debug Mode.", 2, 1);
        task_create(idle, "idle");
        task_create(play_bootup_sequence, "Bootup Music");
        task_create(test_task_a, "a");
        task_create(test_task_b, "b");
    #else
        play_bootup_sequence();
    #endif
    asm volatile("sti");
    sched_start();
    for(;;);
}
