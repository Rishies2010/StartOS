#ifndef LOG_H
#define LOG_H
#include "stdbool.h"

extern bool debug;
void log(const char *fmt, int level, int visibility, ...);

#endif