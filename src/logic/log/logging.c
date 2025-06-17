#include <stddef.h>
#include "../../drv/filesystem.h"
#include "../../drv/vga.h"
#include "../../libk/string.h"
#include "../../libk/qemu.h"

void log(char *logstr, int level, int visibility) {
    static char buffer[FS_SIZE_MAX + 1];
    static char new_log_entry[512];
    char *loglevel;
    switch (level) {
        case 1:
            loglevel = " \t [INFO] - ";
            setcolor(makecolor(150, 150, 150));
            break;
        case 2:
            loglevel = " \t [WARN] - ";
            setcolor(makecolor(255, 90, 0));
            break;
        case 3:
            loglevel = " \t [ERROR] - ";
            setcolor(makecolor(255, 50, 50));
            break;
        case 4:
            loglevel = " \t [PASS] - ";
            setcolor(makecolor(50, 255, 50));
            break;
        default:
            loglevel = " \t [LOGERROR] - ";
            setcolor(makecolor(255, 50, 50));
            break;
    }
    size_t bytes_read = 0;
    if (!fs_read("logfile", buffer, FS_SIZE_MAX, &bytes_read)) {
        setcolor(makecolor(255, 50, 50));
        prints(" \t [LOGERROR] - Failed to read logfile.\n");
        return;
    }
    buffer[bytes_read] = '\0';
    snprintf(new_log_entry, sizeof(new_log_entry), "%s%s\n", loglevel, logstr);
    while (strlen(buffer) + strlen(new_log_entry) >= FS_SIZE_MAX) {
        char *newline = strchr(buffer, '\n');
        if (newline) {
            memmove(buffer, newline + 1, strlen(newline + 1) + 1);
        } else {
            buffer[0] = '\0';
            break;
        }
    }
    strcat(buffer, new_log_entry);
    if (!fs_write("logfile", buffer, strlen(buffer))) {
        setcolor(makecolor(255, 50, 50));
        prints(" \t [LOGERROR] - Failed to write to logfile.\n");
    }
    serial_write_string(new_log_entry);
    if(visibility == 1){
        prints(new_log_entry);}
        setcolor(makecolor(255, 255, 255));
}

void init_log(void) {
    fs_initialize();
    fs_create("logfile");
    log("Logfile Initialized", 1, 0);
}