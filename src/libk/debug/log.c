#include "log.h"
#include <stddef.h>
#include <stdarg.h>
#include "serial.h"
#include "../../drv/vga.h"
#include "../string.h"
#include "../core/mem.h"
#include "../../drv/speaker.h"

void sound_err() {
    speaker_note(0, 0);
    for(volatile int i = 0; i < 2000000; i++);
    speaker_pause();
}

void log(const char* fmt, int level, int visibility, ...) {
    if (!fmt) return;
    const char* loglevel;
    switch (level) {
        case 1: loglevel = "-[INFO] - ";  setcolor(makecolor(150, 150, 150)); break;
        case 2: loglevel = "-[WARN] - ";  setcolor(makecolor(255, 90, 0));    break;
        case 3: loglevel = "-[ERROR] - "; setcolor(makecolor(255, 50, 50)); sound_err(); break;
        case 4: loglevel = "-[PASS] - ";  setcolor(makecolor(50, 255, 50));   break;
        default:loglevel = "-[LOGERROR] - "; setcolor(makecolor(255, 50, 50)); break;
    }
    va_list args;
    va_start(args, visibility);
    char formatted[1024];
    int len = vsnprintf(formatted, sizeof(formatted), fmt, args);
    va_end(args);
    if (len < 0) {
        return;
    }
    size_t loglevel_len = strlen(loglevel);
    size_t formatted_len = strlen(formatted);
    size_t full_len = loglevel_len + formatted_len + 2;
    if (full_len > 2048) {
        return;
    }
    char* logline = (char*)kmalloc(full_len);
    if (!logline) {
        return;
    }
    strcpy(logline, loglevel);
    strcat(logline, formatted);
    strcat(logline, "\n");
    serial_write_string(logline);
    if(visibility == 1){
        prints(logline);
    }
    kfree(logline);
    setcolor(makecolor(255, 255, 255));
}