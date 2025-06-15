#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <stdbool.h>
#include "vga.h"

#define FS_NAME_MAX 32
#define FS_SIZE_MAX 32768
#define FS_MAX_FILES 32
typedef struct {
    char name[FS_NAME_MAX];
    size_t size;
    char data[FS_SIZE_MAX];
    bool exists;
} file_t;

void fs_initialize(void);
bool fs_create(const char* name);
bool fs_delete(const char* name);
bool fs_write(const char* name, const char* data, size_t size);
bool fs_read(const char* name, char* buffer, size_t buffer_size, size_t* bytes_read);
void fs_list(void);
#endif