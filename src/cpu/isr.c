/**
 * This file contains the minimal C code for dispatching
 * interrupts to handlers.
 */

#include "isr.h"
#include "idt.h"
#include "../drv/vga.h"
#include "../drv/pic.h"
#include "../libk/ports.h"
#include "stdint.h"

extern void halt();

isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler)
{
    interrupt_handlers[interrupt] = handler;
}

void isr_handler(registers_t regs)
{
    if(regs.int_no == GENERAL_PROTECTION_FAULT)
    {
        prints("General Protection Fault. Code: ");print_uint(regs.err_code);prints("\n");
    }

    if(interrupt_handlers[regs.int_no])
    {
        prints("Handling ");print_uint(regs.int_no);prints("!\n");
        interrupt_handlers[regs.int_no](regs);
        prints("Returning!\n");
    }
   else {
        prints("Got ISR.\nInterrupt: ");
        print_uint(regs.int_no);
        prints("\n");
   }
}


void irq_handler(registers_t regs)
{
    //If int_no >= 40, we must reset the slave as well as the master
    if(regs.int_no >= 40)
    {
        //reset slave
        outportb(SLAVE_COMMAND, PIC_RESET);
    }

    outportb(MASTER_COMMAND, PIC_RESET);

    if(interrupt_handlers[regs.int_no])
    {
        interrupt_handlers[regs.int_no](regs);
    }
   else {
       if(regs.int_no == 33) return;
       prints("Got IRQ : ");print_uint(regs.int_no);prints(".\n");
   }
}