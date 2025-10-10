/**
 * The StartOS Kernel
 */

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/gfx/tga.h"
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
#include "../drv/disk/sfs.h"

void play_bootup_sequence()
{
    speaker_note(3, 0);
    for (int i = 0; i < 7000000; i++)
        asm volatile("nop");
    speaker_note(3, 2);
    for (int i = 0; i < 7000000; i++)
        asm volatile("nop");
    speaker_note(3, 4);
    for (int i = 0; i < 7000000; i++)
        asm volatile("nop");
    speaker_note(4, 0);
    for (int i = 0; i < 14000000; i++)
        asm volatile("nop");
    speaker_note(4, 7);
    for (int i = 0; i < 7000000; i++)
        asm volatile("nop");
    speaker_note(5, 0);
    for (int i = 0; i < 30000000; i++)
        asm volatile("nop");
    speaker_pause();
}

void idle(void)
{
    while (1)
    {
        asm volatile("hlt");
    }
}

void test_task_a(void) {
    int count = 0;
    uint16_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    log("TaskA CS=0x%x (should be 0x08)", 1, 0, cs);
    
    while(1) {
        if (count % 50000 == 0) {
            log("A", 1, 0);
        }
        count++;
    }
}

void test_task_b(void) {    
    int count = 0;
    while(1) {
        if (count % 5 == 0) {
            log("B", 1, 0);
        }
        count++;
    }
}


void user_task(void)
{
    while(1)
    {
        log("C (I am from Userspace, Ring 3!)", 1, 1);
    }
}

void _start(void)
{
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
    hpet_init(100);
    enable_sse_and_fpu();
    detect_cpu_info(0);
    ata_init();
    sfs_init(0);
    init_keyboard();
    mouse_init();
    init_smp();
    IoApicSetIrq(g_ioApicAddr, 8, 0x28, LocalApicGetId());
    IoApicSetIrq(g_ioApicAddr, 2, 0x22, LocalApicGetId());
    IoApicSetIrq(g_ioApicAddr, 0, 0x20, LocalApicGetId());
    IoApicSetIrq(g_ioApicAddr, 1, 0x21, LocalApicGetId());
    pci_initialize_system();
    e1000_init();

#if debug 
    log("Running In Debug Mode.", 2, 1);
    sfs_list_files();
    print_mem_info(1);
    task_create(idle, "idle");
    task_create(test_task_a, "TaskA");
    task_create(test_task_b, "TaskB");
    task_create_user(user_task, "User Mode Task");
    plotimg("bg.tga", 0, 0);
    ft_run(false);
#else
    task_create(idle, "idle");
    task_create(play_bootup_sequence, "Bootup Music");
    plotimg("bg.tga", 0, 0);
#endif
    sched_start();
    asm volatile("sti");
    idle();
}