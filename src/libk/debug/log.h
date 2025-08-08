#ifndef LOG_H
#define LOG_H
#include "stdbool.h"
#include "../spinlock.h"

#define debug 1
extern char* os_version;
extern spinlock_t loglock;

void log(const char *fmt, int level, int visibility, ...);

#endif
