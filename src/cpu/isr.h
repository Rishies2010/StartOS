#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Exception interrupt numbers
#define DIVISION_BY_ZERO            0
#define DEBUG_EXCEPTION             1
#define NON_MASKABLE_INTERRUPT      2
#define BREAKPOINT_EXCEPTION        3
#define OVERFLOW_EXCEPTION          4
#define BOUND_RANGE_EXCEEDED        5
#define INVALID_OPCODE_EXCEPTION    6
#define DEVICE_NOT_AVAILABLE        7
#define DOUBLE_FAULT                8
#define COPROCESSOR_SEGMENT_OVERRUN 9
#define INVALID_TSS                 10
#define SEGMENT_NOT_PRESENT         11
#define STACK_SEGMENT_FAULT         12
#define GENERAL_PROTECTION_FAULT    13
#define PAGE_FAULT                  14
// 15 is reserved
#define X87_FLOATING_POINT          16
#define ALIGNMENT_CHECK             17
#define MACHINE_CHECK               18
#define SIMD_FLOATING_POINT         19
#define VIRTUALIZATION_EXCEPTION    20
// 21-29 are reserved
#define SECURITY_EXCEPTION          30
// 31 is reserved

// IRQ mappings (remapped to start at 32)
#define IRQ0  32  // System Timer
#define IRQ1  33  // Keyboard
#define IRQ2  34  // Cascade (used internally by PICs)
#define IRQ3  35  // Serial Port 2
#define IRQ4  36  // Serial Port 1
#define IRQ5  37  // Parallel Port 2
#define IRQ6  38  // Floppy Controller
#define IRQ7  39  // Parallel Port 1
#define IRQ8  40  // Real Time Clock
#define IRQ9  41  // Available
#define IRQ10 42  // Available
#define IRQ11 43  // Available
#define IRQ12 44  // PS/2 Mouse
#define IRQ13 45  // Math Coprocessor
#define IRQ14 46  // Primary ATA Hard Disk
#define IRQ15 47  // Secondary ATA Hard Disk

// 64-bit register structure
typedef struct registers
{
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, userrsp, ss;
} registers_t;

// Interrupt service routine handler function type
typedef void (*isr_handler_t)(registers_t*);

// Function prototypes
void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler, const char* handler_name);
void isr_handler(registers_t* regs);
void irq_handler(registers_t* regs);

#endif // ISR_H