#ifndef LOG_H
#define LOG_H

#include "stdbool.h"
#include "../spinlock.h"

#define debug 1
extern char* os_version;

void log_internal(const char* file, int line, const char* fmt, int level, int visibility, ...);

#define log(fmt, level, visibility, ...) \
    log_internal(__FILE__, __LINE__, fmt, level, visibility, ##__VA_ARGS__)

#endif