/**
 * 64-bit interrupt service routine handlers
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
            // prints("Division by Zero Exception!\n");
            break;
        case DEBUG_EXCEPTION:
            // prints("Debug Exception!\n");
            break;
        case NON_MASKABLE_INTERRUPT:
            // prints("Non-Maskable Interrupt!\n");
            break;
        case BREAKPOINT_EXCEPTION:
            // prints("Breakpoint Exception!\n");
            break;
        case INVALID_OPCODE_EXCEPTION:
            // prints("Invalid Opcode Exception!\n");
            break;
        case GENERAL_PROTECTION_FAULT:
            // prints("General Protection Fault! Error Code: ");
            // print_uint(regs->err_code);
            // prints("\n");
            break;
        case PAGE_FAULT:
            // prints("Page Fault! Error Code: ");
            // print_uint(regs->err_code);
            // prints(" CR2: ");
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            // print_uint(cr2);
            // prints("\n");
            break;
        case DOUBLE_FAULT:
            // prints("Double Fault! Error Code: ");
            // print_uint(regs->err_code);
            // prints("\n");
            for(;;)asm volatile("cli; hlt");
            break;
    }

    // Call registered handler if exists
    if(interrupt_handlers[regs->int_no]) {
        // prints("Handling interrupt ");
        // print_uint(regs->int_no);
        // prints("!\n");
        interrupt_handlers[regs->int_no](*regs);
        // prints("Returning from handler!\n");
    } else {
        // prints("Unhandled ISR. Interrupt: ");
        // print_uint(regs->int_no);
        // prints(", Error Code: ");
        // print_uint(regs->err_code);
        // prints("\n");
    }
}

void irq_handler(registers_t* regs)
{
    // Send EOI (End of Interrupt) to PIC
    if(regs->int_no >= 40) {
        // Reset slave PIC
        outportb(SLAVE_COMMAND, PIC_RESET);
    }
    // Reset master PIC
    outportb(MASTER_COMMAND, PIC_RESET);

    // Call registered handler if exists
    if(interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](*regs);
    } else {
        // Don't spam keyboard interrupts
        if(regs->int_no == 33) return;
        
        // prints("Unhandled IRQ: ");
        // print_uint(regs->int_no);
        // prints("\n");
    }
}