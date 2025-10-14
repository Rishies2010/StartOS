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
    uint16_t milliseconds;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

typedef struct
{
    void (*prints)(const char *str);
    void (*printc)(char c);
    void (*ft_run)(bool set);
    void (*setcolor)(uint32_t fg, uint32_t bg);
    void (*plotchar)(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
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
    sfs_error_t (*f_format)(uint8_t drive);
    sfs_error_t (*f_init)(uint8_t drive);
    sfs_error_t (*f_mkdir)(const char* dirname);
    sfs_error_t (*f_rmdir)(const char* dirname);
    sfs_error_t (*f_chdir)(const char* dirname);
    void (*f_get_cwd)(char* buffer, size_t size);
    sfs_error_t (*f_create)(const char* filename, uint32_t size);
    sfs_error_t (*f_seek)(sfs_file_t* file, uint32_t position);
    sfs_error_t (*f_list)(void);
    void (*f_print_stats)(void);
    sfs_error_t (*f_unmount)(void);
    void (*sleep)(uint32_t time);
    void (*sched_yield)(void);
    size_t   (*strlen)(const char* str);
    char*    (*strcpy)(char* dest, const char* src);
    char*    (*strncpy)(char* dest, const char* src, size_t n);
    char*    (*strcat)(char* dest, const char* src);
    char*    (*strncat)(char* dest, const char* src, size_t n);
    int      (*strcmp)(const char* str1, const char* str2);
    int      (*strncmp)(const char* str1, const char* str2, size_t n);
    char*    (*strchr)(const char* str, int c);
    char*    (*strrchr)(const char* str, int c);
    size_t   (*strspn)(const char* str1, const char* str2);
    size_t   (*strcspn)(const char* str1, const char* str2);
    char*    (*strstr)(const char* haystack, const char* needle);
    char*    (*strtok)(char* str, const char* delim);
    char*    (*strtok_r)(char* str, const char* delim, char** saveptr);
    void*    (*memset)(void* ptr, int value, size_t size);
    void*    (*memcpy)(void* dest, const void* src, size_t size);
    void*    (*memmove)(void* dest, const void* src, size_t size);
    int      (*memcmp)(const void* ptr1, const void* ptr2, size_t size);
    void*    (*memchr)(const void* ptr, int value, size_t size);
    int      (*atoi)(const char* str);
    void     (*itoa)(int value, char* str);
    void     (*itoa_hex)(unsigned long value, char* str);
    int      (*vsnprintf)(char* str, size_t size, const char* format, va_list args);
    int      (*snprintf)(char* str, size_t size, const char* format, ...);
    char     (*toLower)(char c);
    char     (*toUpper)(char c);
    uint32_t (*get_pixel_at)(uint32_t x, uint32_t y);
    int (*exec)(const char* filename);
    uint32_t (*mouse_x)(void);
    uint32_t (*mouse_y)(void);
    uint8_t (*mouse_button)(void);
    bool (*mouse_moved)(void);
    void (*mouse_set_pos)(uint32_t x, uint32_t y);
    void (*clr)(void);
    void (*speaker_note)(uint8_t octave, uint8_t note);
    void (*speaker_play)(uint32_t hz);
    void (*speaker_pause)(void);
    rtc_time_t (*rtc_get_time)(void);
    uint32_t (*rtc_calculate_uptime)(const rtc_time_t *start, const rtc_time_t *end);
    rtc_time_t (*rtc_boottime)(void);
    int (*net_send_packet)(void* data, size_t len);
    int (*net_receive_packet)(void* buf, size_t buf_size);
    void (*net_get_mac_address)(uint8_t* mac);
    uint32_t (*net_link_up)(void);
    void (*net_enable_interrupts)(void);
    void (*net_disable_interrupts)(void);
    uint32_t (*net_get_interrupt_status)(void);
    void (*net_handle_interrupt)(void);
} kernel_api_t;

#define log(fmt, level, vis, ...) g_api->log_internal(__FILE__, __LINE__, fmt, level, vis, ##__VA_ARGS__)
#define prints(str) g_api->prints(str)
#define printc(c) g_api->printc(c)
#define ft_run(set) g_api->ft_run(set)
#define setcolor(fg, bg) g_api->setcolor(fg, bg)
#define plotchar(c, x, y, fg, bg) g_api->plotchar(c, x, y, fg, bg)
#define kmalloc(size) g_api->kmalloc(size)
#define kfree(ptr) g_api->kfree(ptr)
#define put_pixel(x, y, color) g_api->put_pixel(x, y, color)
#define read_line(buffer, max_size, print) g_api->read_line(buffer, max_size, print)
#define get_key() g_api->get_key()
#define wait_for_key() g_api->wait_for_key()
#define is_key_pressed(scancode) g_api->is_key_pressed(scancode)
#define is_shift_pressed() g_api->is_shift_pressed()
#define is_ctrl_pressed() g_api->is_ctrl_pressed()
#define is_alt_pressed() g_api->is_alt_pressed()
#define is_caps_lock_on() g_api->is_caps_lock_on()
#define f_format(drive) g_api->f_format(drive)
#define f_init(drive) g_api->f_init(drive)
#define f_mkdir(dirname) g_api->f_mkdir(dirname)
#define f_rmdir(dirname) g_api->f_rmdir(dirname)
#define f_chdir(dirname) g_api->f_chdir(dirname)
#define f_get_cwd(buffer, size) g_api->f_get_cwd(buffer, size)
#define f_create(filename, size) g_api->f_create(filename, size)
#define f_open(filename, file) g_api->f_open(filename, file)
#define f_read(file, buffer, size, br) g_api->f_read(file, buffer, size, br)
#define f_write(file, buffer, size) g_api->f_write(file, buffer, size)
#define f_close(file) g_api->f_close(file)
#define f_rm(filename) g_api->f_rm(filename)
#define f_mk(filename, size) g_api->f_mk(filename, size)
#define f_seek(file, position) g_api->f_seek(file, position)
#define f_list() g_api->f_list()
#define f_print_stats() g_api->f_print_stats()
#define f_unmount() g_api->f_unmount()
#define sleep(time) g_api->sleep(time)
#define sched_yield() g_api->sched_yield()
#define strlen(str)                  g_api->strlen(str)
#define strcpy(dest, src)            g_api->strcpy(dest, src)
#define strncpy(dest, src, n)        g_api->strncpy(dest, src, n)
#define strcat(dest, src)            g_api->strcat(dest, src)
#define strncat(dest, src, n)        g_api->strncat(dest, src, n)
#define strcmp(str1, str2)           g_api->strcmp(str1, str2)
#define strncmp(str1, str2, n)       g_api->strncmp(str1, str2, n)
#define strchr(str, c)               g_api->strchr(str, c)
#define strrchr(str, c)              g_api->strrchr(str, c)
#define strspn(str1, str2)           g_api->strspn(str1, str2)
#define strcspn(str1, str2)          g_api->strcspn(str1, str2)
#define strstr(hay, needle)          g_api->strstr(hay, needle)
#define strtok(str, delim)           g_api->strtok(str, delim)
#define strtok_r(str, delim, sptr)   g_api->strtok_r(str, delim, sptr)
#define memset(ptr, val, size)       g_api->memset(ptr, val, size)
#define memcpy(dest, src, size)      g_api->memcpy(dest, src, size)
#define memmove(dest, src, size)     g_api->memmove(dest, src, size)
#define memcmp(p1, p2, size)         g_api->memcmp(p1, p2, size)
#define memchr(ptr, val, size)       g_api->memchr(ptr, val, size)
#define atoi(str)                    g_api->atoi(str)
#define itoa(value, str)             g_api->itoa(value, str)
#define itoa_hex(value, str)         g_api->itoa_hex(value, str)
#define vsnprintf(str, size, fmt, a) g_api->vsnprintf(str, size, fmt, a)
#define snprintf(str, size, fmt, ...) g_api->snprintf(str, size, fmt, __VA_ARGS__)
#define toLower(c)                   g_api->toLower(c)
#define toUpper(c)                   g_api->toUpper(c)
#define get_pixel_at(x, y) g_api->get_pixel_at(x, y)
#define exec(filename) g_api->exec(filename)
#define mouse_x(void) g_api->mouse_x(void)
#define mouse_y(void) g_api->mouse_y(void)
#define mouse_button(void) g_api->mouse_button(void)
#define mouse_moved(void) g_api->mouse_moved(void)
#define clr(void) g_api->clr(void)
#define speaker_note(octave, note) g_api->speaker_note(octave, note)
#define speaker_play(hz) g_api->speaker_play(hz)
#define speaker_pause(void) g_api->speaker_pause(void)
#define rtc_get_time(void) g_api->rtc_get_time(void)
#define rtc_calculate_uptime(start, end) g_api->rtc_calculate_uptime(start, end)
#define rtc_boottime(void) g_api->rtc_boottime(void)
#define net_send_packet(data, len) g_api->net_send_packet(data, len)
#define net_receive_packet(buf, buf_size) g_api->net_receive_packet(buf, buf_size)
#define net_get_mac_address(mac) g_api->net_get_mac_address(mac)
#define net_link_up() g_api->net_link_up()
#define net_enable_interrupts() g_api->net_enable_interrupts()
#define net_disable_interrupts() g_api->net_disable_interrupts()
#define net_get_interrupt_status() g_api->net_get_interrupt_status()
#define net_handle_interrupt() g_api->net_handle_interrupt()

#endif