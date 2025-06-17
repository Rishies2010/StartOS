; Multiboot2 header constants
MULTIBOOT2_MAGIC equ 0xe85250d6
MULTIBOOT2_ARCH equ 0  ; i386 architecture
MULTIBOOT2_HEADER_LENGTH equ (multiboot2_header_end - multiboot2_header)
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LENGTH)

section .multiboot2
align 8
multiboot2_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HEADER_LENGTH
    dd MULTIBOOT2_CHECKSUM
    
    align 8
framebuffer_tag_start:
    dw 5        ; type = framebuffer
    dw 0        ; flags
    dd framebuffer_tag_end - framebuffer_tag_start  ; size
    dd 1024     ; width
    dd 768      ; height
    dd 32       ; depth (32-bit color)
framebuffer_tag_end:
    
    align 8
    dw 0        ; type = end
    dw 0        ; flags  
    dd 8        ; size
multiboot2_header_end:

section .bss
align 16
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:

section .text
global start
extern kmain

start:
    mov esp, stack_top
    push ebx
    push eax
    call kmain
    cli
.hang:
    hlt
    jmp .hang