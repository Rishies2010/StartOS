#include "log.h"
#include <stddef.h>
#include <stdarg.h>
#include "serial.h"
#include "../string.h"
#include "../core/mem.h"

void log(const char* fmt, int level, int visibility, ...) {
    if (!fmt) return;
    const char* loglevel;
    switch (level) {
        case 1: loglevel = "\t[INFO] - "; break;
        case 2: loglevel = "\t[WARN] - "; break;
        case 3: loglevel = "\t[ERROR] - "; break;
        case 4: loglevel = "\t[PASS] - "; break;
        default: loglevel = "\t[LOGERROR] - "; break;
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
        //do prints here
    }
    kfree(logline);
}