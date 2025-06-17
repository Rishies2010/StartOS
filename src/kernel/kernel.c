/**
 * The StartOS Kernel
 */
 
#include "../drv/vga.h"
#include "../drv/pic.h"
#include "../drv/pit.h"
#include "../drv/keyboard.h"
#include "../drv/filesystem.h"
#include "../libk/ports.h"
#include "../libk/mem.h"
#include "../libk/qemu.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../logic/log/logging.h"

/* Framebuffer stuff*/

uint64_t framebuffer_addr = 0;
uint32_t framebuffer_width = 0;
uint32_t framebuffer_height = 0;
uint32_t framebuffer_pitch = 0;
uint8_t framebuffer_bpp = 0;

// End framebuffer stuff.

/* Multiboot2 Stuff
   Has the constants and parser and stuff*/

#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot2_info_t {
    uint32_t total_size;
    uint32_t reserved;
    struct multiboot_tag tags[0];
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[0];
};

void parse_multiboot2_info(struct multiboot2_info_t* info) {
    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*)(info + 1);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7))) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
                framebuffer_addr = fb_tag->framebuffer_addr;
                framebuffer_width = fb_tag->framebuffer_width;
                framebuffer_height = fb_tag->framebuffer_height;
                framebuffer_pitch = fb_tag->framebuffer_pitch;
                framebuffer_bpp = fb_tag->framebuffer_bpp;
                break;
            }
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
                uint32_t* mem_lower = (uint32_t*)((uint8_t*)tag + 8);
                uint32_t* mem_upper = (uint32_t*)((uint8_t*)tag + 12);
                break;
            }
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
                char* bootloader_name = (char*)((uint8_t*)tag + 8);
                break;
            }
        }
    }
}

// End Multiboot2 stuff

extern uint32_t endmem;
void shutdown(void){
    outportw(0x604, 0x2000);
    outportw(0xB004, 0x2000);
    outportw(0x4004, 0x3400);
    outportb(0xB2, 0x0F);
    outportw(0x8900, 0x2001);
    outportw(0x8900, 0xFF00);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}

void pit_handler(){
    send_eoi(IRQ0);
}

void kmain(uint32_t magic, struct multiboot2_info_t* info){
    parse_multiboot2_info(info);
    init_kernel_heap((uint32_t)&endmem, 0x100000);
    init_log();
    log("Memory Initialized", 1, 0);
    init_gdt();
    remap_pic();
    init_idt();
    init_pit(10);
    serial_init();
    keyboard_initialize();
    prints("\n -> Welcome to StartOS !\n");
    register_interrupt_handler(IRQ0, pit_handler, "PIT Handler");
    asm volatile("sti");
    for(;;)asm volatile("hlt");
}
