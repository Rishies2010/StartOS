#include "filesystem.h"
#include "../libk/string.h"
#include "../libk/mem.h"
#include "../logic/log/logging.h"

// This is a ultra simple RAM Filesystem Simulation

static file_t files[FS_MAX_FILES];
void fs_initialize(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].exists = false;}}
static file_t* find_file(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].exists && strcmp(files[i].name, name) == 0) {
            return &files[i];}}
    return NULL;}
static file_t* find_free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].exists) {
            return &files[i];
        }
    }
    log("RAMFS Out Of Free File Slots", 3, 0);
    return NULL;}
bool fs_create(const char* name) {
    if (find_file(name) != NULL) {
        log("Attempt to create existing file.", 1, 0);
        return false;
    }
    file_t* file = find_free_slot();
    if (file == NULL) {
        return false;
    }
    strcpy(file->name, name);
    file->size = 0;
    file->exists = true;
    return true;
}
bool fs_delete(const char* name) {
    file_t* file = find_file(name);
    if (file == NULL) {
        return false;
    }
    file->exists = false;
    return true;
}
bool fs_write(const char* name, const char* data, size_t size) {
    file_t* file = find_file(name);
    if (file == NULL) {
        return false;
    }
    if (size > FS_SIZE_MAX) {
        return false;
    }
    memcpy(file->data, data, size);
    file->size = size;
    return true;
}
bool fs_read(const char* name, char* buffer, size_t buffer_size, size_t* bytes_read) {
    file_t* file = find_file(name);
    if (file == NULL) {
        log("Attempt to read non-existent file.", 2, 0);
        return false;
    }
    size_t size = file->size;
    if (size > buffer_size) {
        size = buffer_size;
    }
    memcpy(buffer, file->data, size);
    *bytes_read = size;
    
    return true;
}
void fs_list(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].exists) {
            count++;
        }
    }
    if (count == 0) {
        return;
    }
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].exists) {
            char size_str[20];
            int j = 0;
            size_t temp = files[i].size;
            if (temp == 0) {
                size_str[j++] = '0';
            } else {
                while (temp > 0) {
                    size_str[j++] = (temp % 10) + '0';
                    temp /= 10;
                }
            }
            size_str[j] = '\0';
            for (int k = 0; k < j / 2; k++) {
                char tmp = size_str[k];
                size_str[k] = size_str[j - k - 1];
                size_str[j - k - 1] = tmp;
            }
            prints(" ");
            prints(files[i].name);
            prints(" (");
            prints(size_str);
            prints(" bytes)\n");
        }
    }
}