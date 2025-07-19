#include "debugshell.h"
#include "../../drv/vga.h"
#include "../../drv/keyboard.h"
#include "../../libk/debug/log.h"
#include "../../libk/ports.h"
#include "../../libk/string.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "../../drv/rtc.h"
#include "../../libk/core/mem.h"
// Global Stuff
static char command_buffer[MAX_COMMAND_LENGTH];

// Parse command into arguments
static int parse_command(char* command, char* args[], int max_args) {
    int arg_count = 0;
    bool in_arg = false;
    while (*command && arg_count < max_args) {
        if (*command == ' ' || *command == '\t') {
            *command = '\0';
            in_arg = false;
        }
        else if (!in_arg) {
            args[arg_count++] = command;
            in_arg = true;
        }
        
        command++;
    }
    
    return arg_count;
}

// Command: version
static void cmd_version(void) {
    prints(" - StartOS Version (");prints(os_version);prints(")\n");
    prints(" - Created by rishiewas2010.\n");
}

// Command: date
static void cmd_date(void) {
    rtc_time_t current_time = rtc_get_time();
    prints(" Current Date: ");
    printc('0' + (current_time.day / 10));
    printc('0' + (current_time.day % 10));
    printc('/');
    printc('0' + (current_time.month / 10));
    printc('0' + (current_time.month % 10));
    printc('/');
    printc('0' + (current_time.year / 10));
    printc('0' + (current_time.year % 10));
    prints("\n");
    prints(" Current Time: ");
    printc('0' + (current_time.hours / 10));
    printc('0' + (current_time.hours % 10));
    printc(':');
    printc('0' + (current_time.minutes / 10));
    printc('0' + (current_time.minutes % 10));
    printc(':');
    printc('0' + (current_time.seconds / 10));
    printc('0' + (current_time.seconds % 10));
    prints("\n");
}

// Command: uptime
static void cmd_uptime(void) {
    rtc_time_t current_time = rtc_get_time();
    rtc_time_t boot_time = rtc_boottime();
    uint32_t uptime_ms = rtc_calculate_uptime(&boot_time, &current_time);
    uint32_t uptime_seconds = uptime_ms / 1000;
    
    uint32_t hours = uptime_seconds / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    uint32_t seconds = uptime_seconds % 60;
    
    prints("System uptime:\n - ");
    if (hours >= 100) {
        printc('0' + (hours / 100));
        printc('0' + ((hours / 10) % 10));
        printc('0' + (hours % 10));
    } else if (hours >= 10) {
        printc('0' + (hours / 10));
        printc('0' + (hours % 10));
    } else {
        printc('0' + hours);
    }
    prints("h ");
    printc('0' + (minutes / 10));
    printc('0' + (minutes % 10));
    prints("m ");
    printc('0' + (seconds / 10));
    printc('0' + (seconds % 10));
    prints("s\n");
}

//Command: meminfo
static void cmd_meminfo(){
    log("=== Memory Usage Info ===\n", 1, 1);
    print_mem_info(1);
}

static void cmd_gdtinfo(void) {
    uint16_t gdt_limit = 0;
    uint64_t gdt_base = 0;
    
    __asm__ volatile("sgdt %0" : "=m"(*(struct { uint16_t limit; uint64_t base; } __attribute__((packed))*) 
                     &(struct { uint16_t limit; uint64_t base; } __attribute__((packed))){ gdt_limit, gdt_base }));
    
    prints("=== Global Descriptor Table Info ===\n");
    prints("GDT Base: 0x");
    print_hex(gdt_base >> 32);
    print_hex(gdt_base & 0xFFFFFFFF);
    prints("\nGDT Limit: 0x");
    print_hex(gdt_limit);
    prints(" (");
    print_uint((gdt_limit + 1) / 8);
    prints(" entries)\n\nGDT Entries:\n");
    
    const char* segment_names[] = {
        "NULL Segment",
        "Kernel Code (64-bit)",
        "Kernel Data", 
        "User Code (64-bit)",
        "User Data",
        "TSS (Low)",
        "TSS (High)"
    };
    
    uint64_t* gdt_entries = (uint64_t*)gdt_base;
    int max_entries = (gdt_limit + 1) / 8;
    if(max_entries > 7) max_entries = 7;
    
    for(int i = 0; i < max_entries; i++) {
        uint64_t entry = gdt_entries[i];
        
        uint32_t base_low = (entry >> 16) & 0xFFFF;
        uint32_t base_mid = (entry >> 32) & 0xFF;
        uint32_t base_high = (entry >> 56) & 0xFF;
        uint32_t base = base_low | (base_mid << 16) | (base_high << 24);
        
        uint32_t limit_low = entry & 0xFFFF;
        uint32_t limit_high = (entry >> 48) & 0x0F;
        uint32_t limit = limit_low | (limit_high << 16);
        
        uint8_t access = (entry >> 40) & 0xFF;
        uint8_t granularity = (entry >> 52) & 0xFF;
        
        prints("[");
        print_uint(i);
        prints("] ");
        prints(segment_names[i]);
        prints("\n    Base: 0x");
        print_hex(base);
        prints(", Limit: 0x");
        print_hex(limit);
        prints("\n    Access: 0x");
        print_hex(access);
        prints(", Granularity: 0x");
        print_hex(granularity);
        prints("\n    Present: ");
        if(access & 0x80) prints("Yes");
        else prints("No");
        
        if(i > 0 && i < 5) {
            prints(", DPL: ");
            print_uint((access >> 5) & 0x3);
            if(access & 0x08) prints(", Code Segment");
            else prints(", Data Segment");
            if(access & 0x04) prints(", Executable");
            if(access & 0x02) prints(", Writable/Readable");
        }
        prints("\n\n");
    }
}

static void cmd_cpuinfo(void) {
    prints("=== CPU Information ===\n");
    
    uint32_t eax, ebx, ecx, edx;
    char vendor[13] = {0};
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    *(uint32_t*)vendor = ebx;
    *(uint32_t*)(vendor + 4) = edx;
    *(uint32_t*)(vendor + 8) = ecx;
    
    prints("CPU Vendor: ");
    prints(vendor);
    prints("\nMax CPUID Level: ");
    print_uint(eax);
    prints("\n");
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    prints("Family: ");
    print_uint((eax >> 8) & 0xF);
    prints(", Model: ");
    print_uint((eax >> 4) & 0xF);
    prints(", Stepping: ");
    print_uint(eax & 0xF);
    prints("\nFeatures: ");
    
    if(edx & (1 << 0)) prints("FPU ");
    if(edx & (1 << 4)) prints("TSC ");
    if(edx & (1 << 5)) prints("MSR ");
    if(edx & (1 << 6)) prints("PAE ");
    if(edx & (1 << 8)) prints("CX8 ");
    if(edx & (1 << 11)) prints("SEP ");
    if(edx & (1 << 13)) prints("PGE ");
    if(edx & (1 << 15)) prints("CMOV ");
    if(edx & (1 << 23)) prints("MMX ");
    if(edx & (1 << 25)) prints("SSE ");
    if(edx & (1 << 26)) prints("SSE2 ");
    if(ecx & (1 << 0)) prints("SSE3 ");
    if(ecx & (1 << 9)) prints("SSSE3 ");
    if(ecx & (1 << 19)) prints("SSE4.1 ");
    if(ecx & (1 << 20)) prints("SSE4.2 ");
    prints("\n");
}

static void cmd_reginfo(void) {
    prints("=== Memory & Control Registers ===\n");
    
    uint64_t cr0, cr2, cr3, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    
    prints("CR0 (Control):    0x");
    print_hex(cr0 >> 32);
    print_hex(cr0 & 0xFFFFFFFF);
    prints(" ");
    if(cr0 & 1) prints("(Protected Mode) ");
    if(cr0 & 0x80000000) prints("(Paging ON)\n");
    else prints("(Paging OFF)\n");
    
    prints("CR2 (Page Fault): 0x");
    print_hex(cr2 >> 32);
    print_hex(cr2 & 0xFFFFFFFF);
    prints("\nCR3 (Page Dir):   0x");
    print_hex(cr3 >> 32);
    print_hex(cr3 & 0xFFFFFFFF);
    prints("\nCR4 (Extensions): 0x");
    print_hex(cr4 >> 32);
    print_hex(cr4 & 0xFFFFFFFF);
    prints("\n");
}

static void cmd_picinfo(void) {
    prints("=== PIC Controller Info ===\n");
    
    uint8_t master_mask = inportb(0x21);
    uint8_t slave_mask = inportb(0xA1);
    
    prints("Master PIC Mask: 0x");
    print_hex(master_mask);
    prints(" (IRQs ");
    for(int i = 0; i < 8; i++) {
        if(!(master_mask & (1 << i))) {
            print_uint(i);
            prints(" ");
        }
    }
    prints("enabled)\n");
    
    prints("Slave PIC Mask:  0x");
    print_hex(slave_mask);
    prints(" (IRQs ");
    for(int i = 0; i < 8; i++) {
        if(!(slave_mask & (1 << i))) {
            print_uint(i + 8);
            prints(" ");
        }
    }
    prints("enabled)\n");
    
    outportb(0x20, 0x0B);
    uint8_t master_isr = inportb(0x20);
    outportb(0xA0, 0x0B);
    uint8_t slave_isr = inportb(0xA0);
    
    prints("Master ISR: 0x");
    print_hex(master_isr);
    prints("\nSlave ISR:  0x");
    print_hex(slave_isr);
    prints("\n");
}

// Command: echo
static void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        prints(argv[i]);
        if (i < argc - 1) {
            printc(' ');
        }
    }
    printc('\n');
}

// Command: font
void cmd_font(int argc, char* argv[]) {
    if(argc>2){ log(" Usage: font (1, 2 or 3).", 3, 1); return;}
    int fnum = atoi(argv[1]);
    if(!(fnum>0) || !(fnum<4)){ log(" Parameter not a number.", 3, 1); return;}
    prints(" Font set to %i.", fnum);
    setfont(fnum);
}

// Command: shutdown
static void cmd_shutdown(void) {
    clr();
    outportw(0x604, 0x2000);
    outportw(0xB004, 0x2000);
    outportw(0x4004, 0x3400);
    outportb(0xB2, 0x0F);
    outportw(0x8900, 0x2001);
    outportw(0x8900, 0xFF00);
    __asm__ __volatile__("cli");
    __asm__ __volatile__("hlt");
}

// Execute a command
bool shell_execute(const char* command) {
    char cmd_copy[MAX_COMMAND_LENGTH];
    strcpy(cmd_copy, command);
    char* argv[MAX_ARGS];
    int argc = parse_command(cmd_copy, argv, MAX_ARGS);
    
    if(!debug)log("Command Executed: %s.", 1, 0, command);
    if (argc == 0) {
        return true;
    } else if (strcmp(argv[0], "clear") == 0) {
        clr();
    } else if (strcmp(argv[0], "echo") == 0) {
        cmd_echo(argc, argv);
    } else if (strcmp(argv[0], "version") == 0) {
        cmd_version();
    } else if (strcmp(argv[0], "date") == 0) {
        cmd_date();
    } else if (strcmp(argv[0], "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(argv[0], "gdtinfo") == 0) {
        cmd_gdtinfo();
    } else if (strcmp(argv[0], "cpuinfo") == 0) {
        cmd_cpuinfo();
    } else if (strcmp(argv[0], "picinfo") == 0) {
        cmd_picinfo();
    } else if (strcmp(argv[0], "reginfo") == 0) {
        cmd_reginfo();
    } else if (strcmp(argv[0], "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmp(argv[0], "font") == 0) {
        cmd_font(argc, argv);
    } else if (strcmp(argv[0], "shutdown") == 0 || strcmp(argv[0], "exit") == 0) {
        cmd_shutdown();
        return false; // Stop the shell
    } else {
        log(" Unknown command: %s.", 3, 1, argv[0]);
    }
    return true;
}

// Run the shell
void shell_run(void) {
    while (1) {
        prints("\nStartOS:~$ ");
        keyboard_readline(command_buffer, MAX_COMMAND_LENGTH);
        printc('\n');
        if (!shell_execute(command_buffer)) {
            break;
        }
        memset(command_buffer, 0, MAX_COMMAND_LENGTH);
    }
}