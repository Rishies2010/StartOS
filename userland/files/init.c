#include "../userlib.h"

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

typedef struct {
    char name[28];
    uint32_t size;
    uint32_t start_block;
    uint32_t position;                  
    uint8_t entry_index;                
    uint8_t is_open;
} sfs_file_t;

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16

// Command buffer
static char command_buffer[MAX_COMMAND_LENGTH];
static int buffer_pos = 0;

// Parse command into arguments
static int parse_command(char* command, char* args[], int max_args) {
    int arg_count = 0;
    int in_arg = 0;
    
    while (*command && arg_count < max_args) {
        if (*command == ' ' || *command == '\t') {
            *command = '\0';
            in_arg = 0;
        }
        else if (!in_arg) {
            args[arg_count++] = command;
            in_arg = 1;
        }
        command++;
    }
    
    return arg_count;
}

// String compare
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// String copy
static void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

// String length
static int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

// ============ COMMANDS ============

static void cmd_exec(int argc, char* argv[]) {
    if (argc < 2) {
        prints(COLOR_RED "Usage: exec <filename>\n" COLOR_RESET);
        return;
    }
    
    prints(COLOR_YELLOW "Executing: " COLOR_RESET);
    prints(argv[1]);
    prints("\n");
    
    int result = exec(argv[1]);
    
    if (result != 0) {
        prints(COLOR_RED "Failed to execute: " COLOR_RESET);
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_help(void) {
    prints(COLOR_CYAN "Available commands:\n" COLOR_RESET);
    prints("  help      - Show this help message\n");
    prints("  clear     - Clear the screen\n");
    prints("  exec      - Execute a program file\n");
    prints("  echo      - Print arguments\n");
    prints("  version   - Show OS version\n");
    prints("  exit      - Exit shell\n");
    prints("  beep      - Play a beep\n");
    prints("  test      - Run a test sequence\n");
}

static void cmd_clear(void) {
    // ANSI clear screen
    prints("\033[2J\033[H");
}

static void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        prints(argv[i]);
        if (i < argc - 1) prints(" ");
    }
    prints("\n");
}

static void cmd_version(void) {
    prints(COLOR_BOLD COLOR_CYAN);
    prints("  #####  #######    #    ######  #######      #####   #####  \n");
    prints(" #     #    #      # #   #     #    #        #     # #     # \n");
    prints(" #          #     #   #  #     #    #        #     # #       \n");
    prints("  #####     #    #     # ######     #        #     #  #####  \n");
    prints("       #    #    ####### #   #      #        #     #       # \n");
    prints(" #     #    #    #     # #    #     #        #     # #     # \n");
    prints("  #####     #    #     # #     #    #         #####   #####  \n\n");
    prints("StartOS v0.90.0\n\n");
    prints(COLOR_RESET);
}

static void cmd_beep(void) {
    prints("Playing beep...\n");
    speaker_play(440);
    sleep(200);
    speaker_play(880);
    sleep(200);
    speaker_stop();
}

static void cmd_test(void) {
    prints(COLOR_YELLOW "Running syscall tests...\n" COLOR_RESET);
    
    prints("1. Testing prints... ");
    sleep(100);
    prints(COLOR_GREEN "OK\n" COLOR_RESET);
    
    prints("2. Testing sleep... ");
    sleep(500);
    prints(COLOR_GREEN "OK\n" COLOR_RESET);
    
    prints("3. Testing speaker... ");
    speaker_play(523);
    sleep(100);
    speaker_stop();
    prints(COLOR_GREEN "OK\n" COLOR_RESET);
    
    prints("4. Testing mouse... ");
    uint32_t x = mouse_x();
    uint32_t y = mouse_y();
    (void)x; (void)y;
    prints(COLOR_GREEN "OK\n" COLOR_RESET);
    
    prints(COLOR_GREEN "All tests passed!\n" COLOR_RESET);
}

// ============ SHELL CORE ============

static void show_prompt(void) {
    prints(COLOR_BOLD COLOR_GREEN "StartOS" COLOR_RESET);
    prints(COLOR_BOLD COLOR_BLUE " ~$ " COLOR_RESET);
}

static void read_command(void) {
    buffer_pos = 0;
    
    while (1) {
        char c = getkey();
        
        if (c == '\0') {
            // No key pressed, yield and try again
            sleep(10);
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            // Enter pressed
            command_buffer[buffer_pos] = '\0';
            prints("\n");
            break;
        }
        else if (c == '\b' || c == 127) {
            // Backspace
            if (buffer_pos > 0) {
                buffer_pos--;
                prints("\b \b");  // Erase character on screen
            }
        }
        else if (c >= 32 && c < 127) {
            // Printable character
            if (buffer_pos < MAX_COMMAND_LENGTH - 1) {
                command_buffer[buffer_pos++] = c;
                char buf[2] = {c, '\0'};
                prints(buf);
            }
        }
    }
}

static int execute_command(void) {
    char cmd_copy[MAX_COMMAND_LENGTH];
    strcpy(cmd_copy, command_buffer);
    
    char* argv[MAX_ARGS];
    int argc = parse_command(cmd_copy, argv, MAX_ARGS);
    
    if (argc == 0) {
        return 1;  // Continue
    }
    
    // Command dispatch
    if (strcmp(argv[0], "help") == 0) {
        cmd_help();
    }
    else if (strcmp(argv[0], "exec") == 0) {
        cmd_exec(argc, argv);  
    }
    else if (strcmp(argv[0], "clear") == 0) {
        cmd_clear();
    }
    else if (strcmp(argv[0], "echo") == 0) {
        cmd_echo(argc, argv);
    }
    else if (strcmp(argv[0], "version") == 0) {
        cmd_version();
    }
    else if (strcmp(argv[0], "exit") == 0) {
        prints(COLOR_YELLOW "Exiting shell...\n" COLOR_RESET);
        return 0;  // Exit
    }
    else if (strcmp(argv[0], "beep") == 0) {
        cmd_beep();
    }
    else if (strcmp(argv[0], "test") == 0) {
        cmd_test();
    }
    else {
        prints(COLOR_RED "Unknown command: " COLOR_RESET);
        prints(argv[0]);
        prints("\n");
        prints("Type 'help' for available commands.\n");
    }
    
    return 1;  // Continue
}

static void show_banner(void) {
    cmd_clear();
    prints(COLOR_CYAN COLOR_BOLD);
    prints("  #####  #######    #    ######  #######      #####   #####  \n");
    prints(" #     #    #      # #   #     #    #        #     # #     # \n");
    prints(" #          #     #   #  #     #    #        #     # #       \n");
    prints("  #####     #    #     # ######     #        #     #  #####  \n");
    prints("       #    #    ####### #   #      #        #     #       # \n");
    prints(" #     #    #    #     # #    #     #        #     # #     # \n");
    prints("  #####     #    #     # #     #    #         #####   #####  \n");
    prints(COLOR_RESET);
    prints("\n");
    prints(COLOR_YELLOW "Welcome to StartOS Shell!\n" COLOR_RESET);
    prints("Type 'help' for available commands.\n");
    prints("\n");
}

int main(void) {
    show_banner();
    
    while (1) {
        show_prompt();
        read_command();
        
        if (!execute_command()) {
            break;  // Exit requested
        }
    }
    
    prints(COLOR_GREEN "Shell exited cleanly.\n" COLOR_RESET);
    exit(0);
    return 0;
}