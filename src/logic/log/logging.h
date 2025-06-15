#ifndef LOGGING_H
#define LOGGING_H

/**
* This file deals with logging main activities of FlintOS.
* Additionally, it is stored to a file on the RAMDisk.
* Logs are of 3 levels here:
*      - INFO
*      - WARN
*      - ERROR
*      - PASS
*/

void log(char *logstr, int level, int visibility);
void init_log(void);

#endif