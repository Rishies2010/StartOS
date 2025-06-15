ENTRY(start)

SECTIONS
{
    . = 1M;
    
    .multiboot :
    {
        *(.multiboot)
    }
    
    .text :
    {
        *(.text*)
        *(.rodata)
    }
    
    .data :
    {
        *(.data)
    }
    
    .bss :
    {
        *(COMMON)
        *(.bss)
    }

    endmem = .;
}