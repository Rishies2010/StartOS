[BITS 64]
section .text
global task_switch

task_switch:
    mov [rdi + 72 + 48], rax
    mov [rdi + 72 + 40], rbx
    mov [rdi + 72 + 32], rcx
    mov [rdi + 72 + 24], rdx
    mov [rdi + 72 + 16], rsi
    mov [rdi + 72 + 8], rdi
    mov [rdi + 72 + 0], rbp
    mov [rdi + 8 + 0], r15
    mov [rdi + 8 + 8], r14
    mov [rdi + 8 + 16], r13
    mov [rdi + 8 + 24], r12
    mov [rdi + 8 + 32], r11
    mov [rdi + 8 + 40], r10
    mov [rdi + 8 + 48], r9
    mov [rdi + 8 + 56], r8
    mov rax, rdi
    mov [rax + 72 + 8], rax
    mov [rax + 72 + 16], rsi
    lea rax, [rsp + 8]
    mov [rdi + 144 + 24], rax
    mov rax, [rsp]
    mov [rdi + 144 + 0], rax
    pushfq
    pop rax
    mov [rdi + 144 + 16], rax
    mov r15, [rsi + 8 + 0]
    mov r14, [rsi + 8 + 8]
    mov r13, [rsi + 8 + 16]
    mov r12, [rsi + 8 + 24]
    mov r11, [rsi + 8 + 32]
    mov r10, [rsi + 8 + 40]
    mov r9,  [rsi + 8 + 48]
    mov r8,  [rsi + 8 + 56]
    mov rax, [rsi + 72 + 48]
    mov rbx, [rsi + 72 + 40]
    mov rcx, [rsi + 72 + 32]
    mov rdx, [rsi + 72 + 24]
    mov rdi, [rsi + 72 + 8]
    mov rbp, [rsi + 72 + 0]
    mov rsp, [rsi + 144 + 24]
    push qword [rsi + 144 + 16]
    popfq
    push qword [rsi + 144 + 0]
    mov rsi, [rsi + 72 + 16]
    ret