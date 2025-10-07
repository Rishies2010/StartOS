; task_switch.asm
; Context switching for x86_64
; void task_switch(registers_t* old_regs, registers_t* new_regs);

[BITS 64]
section .text
global task_switch

task_switch:
    ; RDI = old_regs pointer (1st arg)
    ; RSI = new_regs pointer (2nd arg)
    
    ; Save current task's registers to old_regs
    ; registers_t structure offsets (from isr.h):
    ; 0: ds (8 bytes)
    ; 8: r15, r14, r13, r12, r11, r10, r9, r8 (64 bytes)
    ; 72: rbp, rdi, rsi, rdx, rcx, rbx, rax (56 bytes)
    ; 128: int_no, err_code (16 bytes)
    ; 144: rip, cs, rflags, userrsp, ss (40 bytes)
    
    ; Save general purpose registers
    mov [rdi + 72 + 48], rax    ; rax
    mov [rdi + 72 + 40], rbx    ; rbx
    mov [rdi + 72 + 32], rcx    ; rcx
    mov [rdi + 72 + 24], rdx    ; rdx
    mov [rdi + 72 + 16], rsi    ; rsi (we'll fix this later)
    mov [rdi + 72 + 8], rdi     ; rdi (we'll fix this later)
    mov [rdi + 72 + 0], rbp     ; rbp
    
    ; Save extended registers
    mov [rdi + 8 + 56], r8
    mov [rdi + 8 + 48], r9
    mov [rdi + 8 + 40], r10
    mov [rdi + 8 + 32], r11
    mov [rdi + 8 + 24], r12
    mov [rdi + 8 + 16], r13
    mov [rdi + 8 + 8], r14
    mov [rdi + 8 + 0], r15
    
    ; Save segment register
    mov ax, ds
    mov [rdi + 0], rax          ; ds
    
    ; Fix the saved RDI and RSI (they were used as arguments)
    mov rax, rdi
    mov [rax + 72 + 8], rax     ; save original rdi
    mov [rax + 72 + 16], rsi    ; save original rsi
    
    ; Save stack pointer (before we push return address)
    mov rax, rsp
    add rax, 8                  ; account for return address on stack
    mov [rdi + 144 + 24], rax   ; userrsp
    
    ; Save return address as RIP
    mov rax, [rsp]
    mov [rdi + 144 + 0], rax    ; rip
    
    ; Save RFLAGS
    pushfq
    pop rax
    mov [rdi + 144 + 16], rax   ; rflags
    
    ; Save code segment
    mov ax, cs
    mov [rdi + 144 + 8], rax    ; cs
    
    ; Save stack segment
    mov ax, ss
    mov [rdi + 144 + 32], rax   ; ss
    
    ; ===== Now restore new task's registers from new_regs =====
    
    ; Restore segment register
    mov rax, [rsi + 0]
    mov ds, ax
    mov es, ax
    
    ; Restore extended registers
    mov r15, [rsi + 8 + 0]
    mov r14, [rsi + 8 + 8]
    mov r13, [rsi + 8 + 16]
    mov r12, [rsi + 8 + 24]
    mov r11, [rsi + 8 + 32]
    mov r10, [rsi + 8 + 40]
    mov r9,  [rsi + 8 + 48]
    mov r8,  [rsi + 8 + 56]
    
    ; Restore general purpose registers (except RSP, we'll do that last)
    mov rax, [rsi + 72 + 48]
    mov rbx, [rsi + 72 + 40]
    mov rcx, [rsi + 72 + 32]
    mov rdx, [rsi + 72 + 24]
    mov rbp, [rsi + 72 + 0]
    
    ; Setup stack to return to new task
    mov rsp, [rsi + 144 + 24]   ; restore stack pointer
    
    ; Push RFLAGS, CS, and RIP for iretq
    push qword [rsi + 144 + 32] ; SS
    push qword [rsi + 144 + 24] ; RSP
    push qword [rsi + 144 + 16] ; RFLAGS
    push qword [rsi + 144 + 8]  ; CS
    push qword [rsi + 144 + 0]  ; RIP
    
    ; Restore RSI and RDI last
    mov rdi, [rsi + 72 + 8]
    mov rax, [rsi + 72 + 16]    ; can't restore rsi yet, we're using it
    mov rsi, rax
    
    ; Jump to new task
    iretq