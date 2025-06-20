#ifndef SHELL_H
#define SHELL_H
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16
void shell_run(void);
bool shell_execute(const char* command);
#endif