/**
 * 64-bit Interrupt Descriptor Table setup
 */

#include "idt.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"
#include <stdint.h>

//IDT entry structure
struct idt_entry_struct
{
    uint16_t base_low;      // Lower 16 bits of handler address
    uint16_t selector;      // Kernel segment selector
    uint8_t  ist;           // Interrupt Stack Table offset (bits 0-2), rest reserved
    uint8_t  flags;         // Type and attributes
    uint16_t base_mid;      // Middle 16 bits of handler address
    uint32_t base_high;     // Upper 32 bits of handler address
    uint32_t reserved;      // Reserved, must be zero
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// IDT pointer structure
struct idt_ptr_struct
{
    uint16_t limit;
    uint64_t base;          // 64-bit base address
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

// External ASM interrupt handlers (ISRs)
extern void isr0();   extern void isr1();   extern void isr2();   extern void isr3();
extern void isr4();   extern void isr5();   extern void isr6();   extern void isr7();
extern void isr8();   extern void isr9();   extern void isr10();  extern void isr11();
extern void isr12();  extern void isr13();  extern void isr14();  extern void isr15();
extern void isr16();  extern void isr17();  extern void isr18();  extern void isr19();
extern void isr20();  extern void isr21();  extern void isr22();  extern void isr23();
extern void isr24();  extern void isr25();  extern void isr26();  extern void isr27();
extern void isr28();  extern void isr29();  extern void isr30();  extern void isr31();

// External ASM interrupt handlers (IRQs)
extern void irq0();   extern void irq1();   extern void irq2();   extern void irq3();
extern void irq4();   extern void irq5();   extern void irq6();   extern void irq7();
extern void irq8();   extern void irq9();   extern void irq10();  extern void irq11();
extern void irq12();  extern void irq13();  extern void irq14();  extern void irq15();

extern void load_idt(idt_ptr_t*);
static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;

void init_idt()
{
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint64_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);

    // Set up exception handlers (ISRs)
    idt_set_gate(0,  (uint64_t)isr0,  0x08, 0x8E);  // Divide by zero
    idt_set_gate(1,  (uint64_t)isr1,  0x08, 0x8E);  // Debug
    idt_set_gate(2,  (uint64_t)isr2,  0x08, 0x8E);  // NMI
    idt_set_gate(3,  (uint64_t)isr3,  0x08, 0x8E);  // Breakpoint
    idt_set_gate(4,  (uint64_t)isr4,  0x08, 0x8E);  // Overflow
    idt_set_gate(5,  (uint64_t)isr5,  0x08, 0x8E);  // Bound range exceeded
    idt_set_gate(6,  (uint64_t)isr6,  0x08, 0x8E);  // Invalid opcode
    idt_set_gate(7,  (uint64_t)isr7,  0x08, 0x8E);  // Device not available
    idt_set_gate(8,  (uint64_t)isr8,  0x08, 0x8E);  // Double fault
    idt_set_gate(9,  (uint64_t)isr9,  0x08, 0x8E);  // Coprocessor segment overrun
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);  // Invalid TSS
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);  // Segment not present
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);  // Stack-segment fault
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);  // General protection fault
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);  // Page fault
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);  // Reserved
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E);  // x87 floating-point exception
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);  // Alignment check
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E);  // Machine check
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);  // SIMD floating-point exception
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E);  // Virtualization exception
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E);  // Control protection exception
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E);  // Reserved
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E);  // Reserved
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E);  // Reserved
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E);  // Reserved
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E);  // Reserved
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E);  // Reserved
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E);  // Reserved
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E);  // Reserved
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E);  // Reserved
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E);  // Reserved

    // Set up hardware interrupt handlers (IRQs)
    idt_set_gate(32, (uint64_t)irq0,  0x08, 0x8E);  // PIT Timer
    idt_set_gate(33, (uint64_t)irq1,  0x08, 0x8E);  // Keyboard
    idt_set_gate(34, (uint64_t)irq2,  0x08, 0x8E);  // Cascade
    idt_set_gate(35, (uint64_t)irq3,  0x08, 0x8E);  // COM2
    idt_set_gate(36, (uint64_t)irq4,  0x08, 0x8E);  // COM1
    idt_set_gate(37, (uint64_t)irq5,  0x08, 0x8E);  // LPT2
    idt_set_gate(38, (uint64_t)irq6,  0x08, 0x8E);  // Floppy disk
    idt_set_gate(39, (uint64_t)irq7,  0x08, 0x8E);  // LPT1
    idt_set_gate(40, (uint64_t)irq8,  0x08, 0x8E);  // CMOS RTC
    idt_set_gate(41, (uint64_t)irq9,  0x08, 0x8E);  // Free for peripherals
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0x8E);  // Free for peripherals
    idt_set_gate(43, (uint64_t)irq11, 0x08, 0x8E);  // Free for peripherals
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E);  // PS2 mouse
    idt_set_gate(45, (uint64_t)irq13, 0x08, 0x8E);  // FPU / coprocessor
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0x8E);  // Primary ATA
    idt_set_gate(47, (uint64_t)irq15, 0x08, 0x8E);  // Secondary ATA

    load_idt(&idt_ptr);
    log("IDT Installed.", 1, 0);
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_mid = (base >> 16) & 0xFFFF;
    idt_entries[num].base_high = (base >> 32) & 0xFFFFFFFF;

    idt_entries[num].selector = sel;
    idt_entries[num].ist = 0;           // Don't use IST for now
    idt_entries[num].flags = flags;
    idt_entries[num].reserved = 0;
}