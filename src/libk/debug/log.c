#include "log.h"
#include <stddef.h>
#include <stdarg.h>
#include "serial.h"
#include "../../drv/vga.h"
#include "../string.h"
#include "../core/mem.h"
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
    int font = current_font;
    setfont(1);
    switch (level) {
        case 1: loglevel = "-[INFO] - ";  setcolor(makecolor(150, 150, 150)); break;
        case 2: loglevel = "-[WARN] - ";  setcolor(makecolor(255, 90, 0));    break;
        case 3: loglevel = "-[ERROR] - "; setcolor(makecolor(255, 50, 50)); if(!debug)sound_err(); break;
        case 4: loglevel = "-[PASS] - ";  setcolor(makecolor(50, 255, 50));   break;
        default:loglevel = "-[LOGERROR] - "; setcolor(makecolor(255, 50, 50));  break;
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
    size_t loglevel_len = strlen(loglevel);
    size_t formatted_len = strlen(formatted);
    size_t full_len = loglevel_len + formatted_len + 2;
    if (full_len > 2048) {
        spinlock_release(&loglock);
        return;
    }
    char* logline = (char*)kmalloc(full_len);
    if (!logline) {
        spinlock_release(&loglock);
        return;
    }
    strcpy(logline, loglevel);
    strcat(logline, formatted);
    strcat(logline, "\n");
    serial_write_string(logline);
    if(visibility == 1 || debug){
        prints(logline);
    }
    kfree(logline);
    setfont(font); 
    setcolor(makecolor(255, 255, 255));
    spinlock_release(&loglock);
}