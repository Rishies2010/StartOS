#include "../drv/vga.h"
#include "../drv/keyboard.h"
#include "../logic/log/logging.h"
#include "../libk/string.h"
#include "shell.h"
#include "../libk/ports.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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

//Get GDT Info.
static void cmd_gdtinfo(void) {
    uint8_t gdtr_data[6];
    asm volatile("sgdt %0" : "=m"(gdtr_data));
    uint16_t limit = gdtr_data[0] | (gdtr_data[1] << 8);
    uint32_t base = gdtr_data[2] | (gdtr_data[3] << 8) | (gdtr_data[4] << 16) | (gdtr_data[5] << 24);
    prints(" -\t Current GDT At: ");print_hex(base);
    prints(", size: ");print_hex(limit);prints(".\n");
    uint64_t* gdt_entries = (uint64_t*)0x10B0;
    for(int i = 0; i < 4; i++) {
        prints(" - GDT Entry ");print_hex(i);prints(": ");print_hex((uint32_t)(gdt_entries[i] >> 32));print_hex((uint32_t)gdt_entries[i]);prints("\n");
    }
}

// Execute a command
bool shell_execute(const char* command) {
    char cmd_copy[MAX_COMMAND_LENGTH];
    strcpy(cmd_copy, command);
    char* argv[MAX_ARGS];
    int argc = parse_command(cmd_copy, argv, MAX_ARGS);
    
    if (argc == 0) {
        return true;
    } if (strcmp(argv[0], "clear") == 0) {
        clr();
    } else if (strcmp(argv[0], "gdt") == 0) {
        cmd_gdtinfo();
    } else if (strcmp(argv[0], "shutdown") == 0 || strcmp(argv[0], "exit") == 0) {
        cmd_shutdown();
        return false;
    } else {
        prints(" No such debug command : %s.", argv[0]);
    }
    log("Command Executed: %s.", 1, 0, command);
    return true;
}

// Run the shell
void shell_run(void) {
    while (1) {
        setcolor(makecolor(100, 255, 100));
        prints("\n StartOS:~$ ");
        setcolor(makecolor(255, 255, 255));
        keyboard_readline(command_buffer, MAX_COMMAND_LENGTH);
        printc('\n');
        if (!shell_execute(command_buffer)) {
            break;
        }
        memset(command_buffer, 0, MAX_COMMAND_LENGTH);
    }
}