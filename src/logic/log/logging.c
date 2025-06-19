#include <stddef.h>
#include "../../drv/filesystem.h"
#include "../../drv/vga.h"
#include "../../libk/string.h"
#include "../../libk/qemu.h"
#include "../../libk/mem.h"

#include <stdarg.h>

void log(const char *fmt, int level, int visibility, ...) {
    const char *loglevel;
    switch (level) {
        case 1: loglevel = " \t [INFO] - ";  setcolor(makecolor(150, 150, 150)); break;
        case 2: loglevel = " \t [WARN] - ";  setcolor(makecolor(255, 90, 0));    break;
        case 3: loglevel = " \t [ERROR] - "; setcolor(makecolor(255, 50, 50));   break;
        case 4: loglevel = " \t [PASS] - ";  setcolor(makecolor(50, 255, 50));   break;
        default:loglevel = " \t [LOGERROR] - "; setcolor(makecolor(255, 50, 50)); break;
    }
    va_list args_copy;
    va_list args;
    va_start(args, visibility);
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *formatted = (char*)kmalloc(len + 1);
    if (!formatted) return;
    vsnprintf(formatted, len + 1, fmt, args_copy);
    va_end(args_copy);
    size_t bytes_read = 0;
    char *buffer = (char*)kmalloc(FS_SIZE_MAX + 1);
    if (!buffer || !fs_read("logfile", buffer, FS_SIZE_MAX, &bytes_read)) {
        setcolor(makecolor(255, 50, 50));
        prints(" \t [LOGERROR] - Failed to read logfile.\n");
        if (buffer) kfree(buffer);
        kfree(formatted);
        return;
    }
    buffer[bytes_read] = '\0';
    size_t full_len = strlen(loglevel) + strlen(formatted) + 2;
    char *logline = (char*)kmalloc(full_len);
    if (!logline) {
        kfree(buffer);
        kfree(formatted);
        return;
    }
    snprintf(logline, full_len, "%s%s\n", loglevel, formatted);
    while (strlen(buffer) + strlen(logline) >= FS_SIZE_MAX) {
        char *newline = strchr(buffer, '\n');
        if (newline) {
            memmove(buffer, newline + 1, strlen(newline + 1) + 1);
        } else {
            buffer[0] = '\0';
            break;
        }
    }
    size_t final_size = strlen(buffer) + strlen(logline) + 1;
    char *final_log = (char*)kmalloc(final_size);
    if (!final_log) {
        kfree(buffer); kfree(formatted); kfree(logline);
        return;
    }
    strcpy(final_log, buffer);
    strcat(final_log, logline);
    if (!fs_write("logfile", final_log, strlen(final_log))) {
        setcolor(makecolor(255, 50, 50));
        prints(" \t [LOGERROR] - Failed to write to logfile.\n");
    }
    serial_write_string(logline);
    if (visibility == 1) prints(logline);
    setcolor(makecolor(255, 255, 255));
    kfree(buffer);
    kfree(formatted);
    kfree(logline);
    kfree(final_log);
}


void init_log(void) {
    fs_initialize();
    fs_create("logfile");
    log("Logfile Initialized", 1, 0);
}