#ifndef DEBUGSHELL_H
#define DEBUGSHELL_H
#include <stdbool.h>
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16
void shell_run(void);
bool shell_execute(const char* command);
#endif