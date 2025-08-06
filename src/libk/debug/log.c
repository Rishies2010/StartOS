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

void log(const char* fmt, int level, int visibility, ...) {
    if (!fmt) return;
    spinlock_acquire(&loglock);

    const char* loglevel;
    const char* color_seq;
    int cpuid = LocalApicGetId();
    
    switch (level) {
        case 1: 
            loglevel = "-[INFO] - "; 
            color_seq = "\x1b[38;2;150;150;150m";
            break;
        case 2: 
            loglevel = "-[WARN] - "; 
            color_seq = "\x1b[38;2;255;90;0m";
            break;
        case 3: 
            loglevel = "-[ERROR] - "; 
            color_seq = "\x1b[38;2;255;50;50m";
            if(!debug) sound_err(); 
            break;
        case 4: 
            loglevel = "-[PASS] - "; 
            color_seq = "\x1b[38;2;50;255;50m";
            break;
        default:
            loglevel = "-[LOGERR] - "; 
            color_seq = "\x1b[38;2;255;50;50m";
            break;
    }

    va_list args;
    va_start(args, visibility);
    char formatted[1024];
    int len = vsnprintf(formatted, sizeof(formatted), fmt, args);
    va_end(args);
    
    if (len < 0) {
        spinlock_release(&loglock);
        return;
    }
    char cpuid_str[16];
    if(cpuid != 0) snprintf(cpuid_str, sizeof(cpuid_str), "[CPU%d] - ", cpuid); else cpuid_str[0] = '\0';

    size_t total_len = strlen(loglevel) + strlen(cpuid_str) + len + 2;
    char* logline = (char*)kmalloc(total_len);
    if (!logline) {
        spinlock_release(&loglock);
        return;
    }
    strcpy(logline, loglevel);
    strcat(logline, cpuid_str);
    strcat(logline, formatted);
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
