#include "isr.h"
#include "idt.h"
#include "../drv/local_apic.h"
#include "../libk/ports.h"
#include "../libk/string.h"
#include "../libk/debug/log.h"
#include <stdint.h>

isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler, const char* handler_name)
{
    interrupt_handlers[interrupt] = handler;
    log("[ISR] New Interrupt Handler Registered : %s.", 1, 0, handler_name);
}

void isr_handler(registers_t* regs)
{
    switch(regs->int_no) {
        case DIVISION_BY_ZERO:
            log("\n=== DIVISION BY ZERO EXCEPTION ===\n", 3, 1);
            log("RIP: 0x%lx, RSP: 0x%lx", 3, 1, regs->rip, regs->userrsp);
            log("RAX: 0x%lx, RBX: 0x%lx, RCX: 0x%lx, RDX: 0x%lx", 3, 1, 
                regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log("\n=== REGISTER DUMP ===\n", 3, 1);
            log("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx", 3, 1,
                regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx RSP=0x%016lx", 3, 1,
                regs->rsi, regs->rdi, regs->rbp, regs->userrsp);
            log("RIP=0x%016lx RFLAGS=0x%016lx", 3, 1, regs->rip, regs->rflags);
            for(;;)asm volatile("cli; hlt");
            break;
            
        case INVALID_OPCODE_EXCEPTION:
            log("\n=== INVALID OPCODE EXCEPTION ===\n", 3, 1);
            log("RIP: 0x%lx (instruction pointer)", 3, 1, regs->rip);
            log("CS: 0x%lx, RFLAGS: 0x%lx", 3, 1, regs->cs, regs->rflags);
            
            uint8_t* bad_instr = (uint8_t*)regs->rip;
            log("Instruction bytes: %02x %02x %02x %02x", 3, 1,
                bad_instr[0], bad_instr[1], bad_instr[2], bad_instr[3]);
            log("\n=== REGISTER DUMP ===\n", 3, 1);
            log("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx", 3, 1,
                regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx RSP=0x%016lx", 3, 1,
                regs->rsi, regs->rdi, regs->rbp, regs->userrsp);
            log("RIP=0x%016lx RFLAGS=0x%016lx", 3, 1, regs->rip, regs->rflags);
            for(;;)asm volatile("cli; hlt");
            break;
            
        case GENERAL_PROTECTION_FAULT:
            log("\n=== GENERAL PROTECTION FAULT ===\n", 3, 1);
            log("Error Code: 0x%lx", 3, 1, regs->err_code);
            
            if(regs->err_code & 1) {
                log("External event caused fault", 3, 1);
            } else {
                log("Internal event caused fault", 3, 1);
            }
            
            uint16_t selector = (regs->err_code >> 3) & 0x1FFF;
            if(selector) {
                log("Segment selector: 0x%x", 3, 1, selector);
            }
            
            log("RIP: 0x%lx, RSP: 0x%lx", 3, 1, regs->rip, regs->userrsp);
            log("\n=== REGISTER DUMP ===\n", 3, 1);
            log("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx", 3, 1,
                regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx RSP=0x%016lx", 3, 1,
                regs->rsi, regs->rdi, regs->rbp, regs->userrsp);
            log("RIP=0x%016lx RFLAGS=0x%016lx", 3, 1, regs->rip, regs->rflags);
            for(;;)asm volatile("cli; hlt");
            break;
            
        case PAGE_FAULT:
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            log("\n=== PAGE FAULT ===\n", 3, 1);
            log("Faulting address: 0x%lx", 3, 1, cr2);
            log("Error code: 0x%lx", 3, 1, regs->err_code);
            
            if(regs->err_code & 1) log("- Page protection violation", 3, 1);
            else log("- Page not present", 3, 1);
            
            if(regs->err_code & 2) log("- Write operation", 3, 1);
            else log("- Read operation", 3, 1);
            
            if(regs->err_code & 4) log("- User mode access", 3, 1);
            else log("- Supervisor mode access", 3, 1);
            
            log("RIP: 0x%lx", 3, 1, regs->rip);
            log("\n=== REGISTER DUMP ===\n", 3, 1);
            log("RAX=0x%016lx RBX=0x%016lx RCX=0x%016lx RDX=0x%016lx", 3, 1,
                regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log("RSI=0x%016lx RDI=0x%016lx RBP=0x%016lx RSP=0x%016lx", 3, 1,
                regs->rsi, regs->rdi, regs->rbp, regs->userrsp);
            log("RIP=0x%016lx RFLAGS=0x%016lx", 3, 1, regs->rip, regs->rflags);
            for(;;)asm volatile("cli; hlt");
            break;
    }

    if(interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](*regs);
    } else {
        log("[ISR] Unhandled ISR. Interrupt: %i, Error Code: %d.", 3, 1, regs->int_no, regs->err_code);
    }
}

void irq_handler(registers_t* regs)
{
    if(interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](*regs);
    } else {
        log("[IRQ] Unhandled IRQ: %d", 3, 1, regs->int_no);
    }
    LocalApicSendEOI();
}