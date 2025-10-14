#ifndef KERNEL_API_H
#define KERNEL_API_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../src/libk/string.h"
#include <stdarg.h>

typedef struct {
    char name[28];
    uint32_t size;
    uint32_t start_block;
    uint32_t position;                  
    uint8_t entry_index;                
    uint8_t is_open;
} sfs_file_t;

typedef enum {
    SFS_OK = 0,
    SFS_ERR_NOT_INITIALIZED,
    SFS_ERR_INVALID_DRIVE,
    SFS_ERR_READ_FAILED,
    SFS_ERR_WRITE_FAILED,
    SFS_ERR_NOT_SFS,
    SFS_ERR_FILE_NOT_FOUND,
    SFS_ERR_NO_SPACE,
    SFS_ERR_ALREADY_EXISTS,
    SFS_ERR_TOO_MANY_ENTRIES,
    SFS_ERR_NOT_OPEN,
    SFS_ERR_EOF,
    SFS_ERR_INVALID_PARAM,
    SFS_ERR_NOT_EMPTY,
    SFS_ERR_NOT_A_DIRECTORY,
    SFS_ERR_IS_DIRECTORY
} sfs_error_t;

typedef struct
{
    void (*prints)(const char *str);
    void (*printc)(char c);
    void (*setcolor)(uint32_t fg, uint32_t bg);
    void (*log_internal)(const char *file, int line, const char *fmt, int level, int visibility, ...);
    void *(*kmalloc)(uint64_t size);
    void (*kfree)(void *ptr);
    void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);
    void (*read_line)(char *buffer, size_t max_size, bool print);
    char (*get_key)(void);
    char (*wait_for_key)(void);
    bool (*is_key_pressed)(uint8_t scancode);
    bool (*is_shift_pressed)(void);
    bool (*is_ctrl_pressed)(void);
    bool (*is_alt_pressed)(void);
    bool (*is_caps_lock_on)(void);
    sfs_error_t (*f_open)(const char* filename, sfs_file_t* file);
    sfs_error_t (*f_read)(sfs_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
    sfs_error_t (*f_write)(sfs_file_t* file, const void* buffer, uint32_t size);
    sfs_error_t (*f_close)(sfs_file_t* file);
    sfs_error_t (*f_mk)(const char* filename, uint32_t size);
    sfs_error_t (*f_rm)(const char* filename);
    void (*sleep)(uint32_t time);
    void (*sched_yield)(void);
    size_t (*strlen)(const char* str);
    char* (*strcpy)(char* dest, const char* src);
    char* (*strcat)(char* dest, const char* src);
    int (*strcmp)(const char* str1, const char* str2);
    int (*atoi)(const char* str);
    void (*itoa)(int value, char* str);
    void (*itoa_hex)(unsigned long value, char* str);
} kernel_api_t;

#define log(fmt, level, vis, ...) api->log_internal(__FILE__, __LINE__, fmt, level, vis, ##__VA_ARGS__)
#define prints(str) api->prints(str)
#define printc(c) api->printc(c)
#define setcolor(fg, bg) api->setcolor(fg, bg)
#define kmalloc(size) api->kmalloc(size)
#define kfree(ptr) api->kfree(ptr)
#define put_pixel(x, y, color) api->put_pixel(x, y, color)
#define read_line(buffer, max_size, print) api->read_line(buffer, max_size, print)
#define get_key() api->get_key()
#define wait_for_key() api->wait_for_key()
#define is_key_pressed(scancode) api->is_key_pressed(scancode)
#define is_shift_pressed() api->is_shift_pressed()
#define is_ctrl_pressed() api->is_ctrl_pressed()
#define is_alt_pressed() api->is_alt_pressed()
#define is_caps_lock_on() api->is_caps_lock_on()
#define f_open(filename, file) api->f_open(filename, file)
#define f_read(file, buffer, size, bytes_read) api->f_read(file, buffer, size, bytes_read)
#define f_write(file, buffer, size) api->f_write(file, buffer, size)
#define f_close(file) api->f_close(file)
#define f_mk(filename, size) api->f_mk(filename, size)
#define f_rm(filename) api->f_rm(filename)
#define sleep(time) api->sleep(time)
#define sched_yield() api->sched_yield()
#define strlen(str) api->strlen(str)
#define strcpy(dest, src) api->strcpy(dest, src)
#define strcat(dest, src) api->strcat(dest, src)
#define strcmp(str1, str2) api->strcmp(str1, str2)
#define atoi(str) api->atoi(str)
#define itoa(value, str) api->itoa(value, str)
#define itoa_hex(value, str) api->itoa_hex(value, str)

#endif