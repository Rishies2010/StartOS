/**
 * Interrupt service routine handlers
 */

#include "isr.h"
#include "idt.h"
#include "../drv/pic.h"
#include "../libk/ports.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"
#include <stdint.h>

isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler, const char* handler_name)
{
    interrupt_handlers[interrupt] = handler;
    log("New Interrupt Handler Registered : %s.", 1, handler_name);
}

void isr_handler(registers_t* regs)
{
    // Handle specific exceptions
    switch(regs->int_no) {
        case DIVISION_BY_ZERO:
            log("Division by Zero Exception!\n", 3);
            break;
        case DEBUG_EXCEPTION:
            log("Debug Exception!\n", 3);
            break;
        case NON_MASKABLE_INTERRUPT:
            log("Non-Maskable Interrupt!\n", 3);
            break;
        case BREAKPOINT_EXCEPTION:
            log("Breakpoint Exception!\n", 3);
            break;
        case INVALID_OPCODE_EXCEPTION:
            log("Invalid Opcode Exception!\n", 3);
            break;
        case GENERAL_PROTECTION_FAULT:
            log("General Protection Fault! Error Code: %d", 3, regs->err_code);
            break;
        case PAGE_FAULT:
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            log("Page Fault! Error Code: %i; CR2: %d.", 3, regs->err_code, cr2);
            break;
        case DOUBLE_FAULT:
            log("Double Fault! Error Code: %i", 3, regs->err_code);
            for(;;)asm volatile("cli; hlt");
            break;
    }

    // Call registered handler if exists
    if(interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](*regs);
    } else {
        log("Unhandled ISR. Interrupt: %i, Error Code: %d.", 3, regs->int_no, regs->err_code);
    }
}

void irq_handler(registers_t* regs)
{
    if(regs->int_no >= 40) {
        outportb(SLAVE_COMMAND, PIC_RESET);
    }
    outportb(MASTER_COMMAND, PIC_RESET);
    if(interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](*regs);
    } else {
        if(regs->int_no == 33) return;
        log("Unhandled IRQ: %d", 3, regs->int_no);
    }
}