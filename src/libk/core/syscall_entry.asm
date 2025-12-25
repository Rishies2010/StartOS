[BITS 64]
section .text
global syscall_entry
extern syscall_handler
extern tss

syscall_entry:
    mov r15, [rel tss + 4]
    xchg rsp, r15
    
    push r15        ; Save user RSP
    push rcx        ; Save user RIP
    push r11        ; Save user RFLAGS
    
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    mov r9, r8
    mov r8, r10
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    
    call syscall_handler
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    pop r11         ; RFLAGS
    pop rcx         ; RIP
    pop r15         ; User RSP into temp register
    
    mov rsp, r15
    
    o64 sysret