/**
 * The StartOS Kernel
 */

// Kernel Includes

#include "../libk/debug/serial.h"
#include "../libk/debug/log.h"
#include "../libk/core/mem.h"
#include "../libk/string.h"
#include "../libk/ports.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/sse.h"
#include "../cpu/isr.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/rtc.h"
#include "../drv/vga.h"
#include "../drv/apic.h"
#include "../drv/disk/ff.h"
#include "../drv/disk/ata_pio_drv.h"

void pit_handler(){
    send_eoi(IRQ0);
}
FATFS fs;       // File system object
FRESULT res;
void format_and_mount(void) {
    // Work buffer for formattingmake qemu
    BYTE work[FF_MAX_SS];
    
    // Format parameters
    MKFS_PARM fmt_params = {
        .fmt = FM_FAT32,        // FAT32 format
        .n_fat = 1,             // Number of FATs
        .align = 0,             // Data area alignment (0 = auto)
        .n_root = 0,            // Number of root entries (0 = auto for FAT32)
        .au_size = 0            // Allocation unit size (0 = auto)
    };
    
    // Format the drive
    res = f_mkfs("0:", &fmt_params, work, sizeof(work));
    if (res == FR_OK) {
        log("Drive formatted successfully", 4, 0);
    } else {
        log("Format failed.\n           ERRCODE : %i", 3, 1, res);
        return;
    }
    
    // Mount the filesystem
    res = f_mount(&fs, "0:", 1);
    if (res == FR_OK) {
        log("Drive mounted successfully", 4, 0);
    } else {
        log("Mount failed.\n           ERRCODE : %i", 3, 1, res);
    }

}

void _start(void){
    serial_init();
    init_pmm();
    init_kernel_heap();
    init_gdt();
    init_idt();
    enable_sse();
    if(init_apic()) {
        apic_timer_init(100);
    } else {
        log("APIC not available, falling back to PIT", 3, 1);
        remap_pic();
        init_pit(10);
        register_interrupt_handler(IRQ0, pit_handler, "PIT Handler");}
    rtc_initialize();
    vga_init();
    asm volatile("sti");
    format_and_mount();
    prints("\n Welcome to StartOS !\n\n");

    uint8_t test_buffer[512];
    ata_status_t test_status = ata_read_sectors(0, 1, test_buffer);
    log("ATA test read result: %d", 4, 1, test_status); 
    for(;;);
}