OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

SECTIONS
{
    . = 0xFFFFFFFF80000000;
    
    .text ALIGN(4K) : AT(0x100000) {
        *(.text*)
    }
    
    .rodata ALIGN(4K) : AT(0x100000 + SIZEOF(.text)) {
        *(.rodata*)
    }
    
    .data ALIGN(4K) : AT(0x100000 + SIZEOF(.text) + SIZEOF(.rodata)) {
        *(.data*)
    }
    
    .bss ALIGN(4K) : AT(0x100000 + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)) {
        *(COMMON)
        *(.bss*)
    }
}