#ifndef LOG_H
#define LOG_H
#include "stdbool.h"

#define debug 0
extern char* os_version;

void log(const char *fmt, int level, int visibility, ...);

#endif