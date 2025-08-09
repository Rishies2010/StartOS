#include "log.h"
#include <stddef.h>
#include <stdarg.h>
#include "serial.h"
#include "../../drv/vga.h"
#include "../string.h"
#include "../core/mem.h"
#include "../../drv/local_apic.h"
#include "../../drv/speaker.h"

spinlock_t loglock;
char* os_version = debug?"0.90.0 DEBUG_ENABLED":"0.90.0 Unstable";

void sound_err() {
    speaker_note(0, 0);
    for(volatile int i = 0; i < 2000000; i++);
    speaker_pause();
}

void log_internal(const char* file, int line, const char* fmt, int level, int visibility, ...) {
    spinlock_acquire(&loglock);

    const char* color_seq;
    int cpuid = LocalApicGetId();
    
    switch (level) {
        case 1:
            color_seq = "\x1b[38;2;150;150;150m";
            break;
        case 2:
            color_seq = "\x1b[38;2;255;90;0m";
            break;
        case 3:
            color_seq = "\x1b[38;2;255;50;50m";
            if(!debug) sound_err();
            break;
        case 4: 
            color_seq = "\x1b[38;2;50;255;50m";
            break;
        default: 
            color_seq = "\x1b[38;2;255;50;50m";
            break;
    }
    va_list args;
    va_start(args, visibility);
    
    char header[256];
    if(level < 1 || level > 4){
        snprintf(header, sizeof(header), "KERNEL PANIC !\n\n At : %s:%d.\n\n ERROR MESSAGE : ", file, line);
        __asm__ __volatile__("cli");
        for(;;)__asm__ __volatile__("hlt");
    }
    else
        snprintf(header, sizeof(header), "[%s:%d]- ", file, line);
    
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    char cpuid_str[16];
    if(cpuid != 0) snprintf(cpuid_str, sizeof(cpuid_str), "[CPU%d]- ", cpuid); else cpuid_str[0] = '\0';

    size_t total_len = strlen(header) + strlen(cpuid_str) + strlen(message) + 2;
    char* logline = kmalloc(total_len);
    if (!logline) {
        spinlock_release(&loglock);
        return;
    }
    
    strcpy(logline, header);
    strcat(logline, cpuid_str);
    strcat(logline, message);
    strcat(logline, "\n");

    serial_write_string(color_seq);
    serial_write_string(logline);
    serial_write_string("\x1b[0m");
    
    if (visibility == 1 || debug) {
        prints(color_seq);
        prints(logline);
        prints("\x1b[0m");
    }
    
    kfree(logline);
    spinlock_release(&loglock);
}