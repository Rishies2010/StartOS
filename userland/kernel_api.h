#ifndef KERNEL_API_H
#define KERNEL_API_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../src/drv/disk/sfs.h"
#include "../src/drv/rtc.h"
#include "../src/drv/vga.h"
#include "../src/libk/string.h"
#include "../src/kernel/sched.h"
#include "../src/libk/core/socket.h"
#include <stdarg.h>

typedef struct
{
    void (*prints)(const char *str);
    void (*printc)(char c);
    void (*ft_run)(bool set);
    void (*setcolor)(uint32_t fg, uint32_t bg);
    void (*plotchar)(char c, uint32_t x, uint32_t y, uint32_t fg);
    void (*draw_text_at)(const char *str, uint32_t x, uint32_t y, uint32_t color);
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
    sfs_error_t (*f_open)(bool fs_select, const char* filename, void** handle);
    sfs_error_t (*f_read)(bool fs_select, void* handle, void* buffer, uint32_t size, uint32_t* bytes_read);
    sfs_error_t (*f_write)(bool fs_select, void* handle, const void* buffer, uint32_t size);
    sfs_error_t (*f_close)(bool fs_select, void* handle);
    sfs_error_t (*f_create)(bool fs_select, const char* filename, uint32_t size);
    sfs_error_t (*f_rm)(bool fs_select, const char* filename);
    sfs_error_t (*f_seek)(bool fs_select, void* handle, uint32_t position);
    sfs_error_t (*f_format)(uint8_t drive);
    sfs_error_t (*f_init)(uint8_t drive);
    sfs_error_t (*f_mkdir)(const char* dirname);
    sfs_error_t (*f_rmdir)(const char* dirname);
    sfs_error_t (*f_chdir)(const char* dirname);
    void (*f_get_cwd)(char* buffer, size_t size);
    sfs_error_t (*f_list)(void);
    void (*f_print_stats)(void);
    sfs_error_t (*f_unmount)(void);
    void (*sleep)(uint32_t time);
    void (*sched_yield)(void);
    task_t *(*task_create)(void (*entry)(void), const char *name);
    task_t *(*task_create_user)(void (*entry)(void), const char *name); 
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
    int (*exec)(const char* filename, int argc, char **argv);
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
    socket_error_t (*socket_create)(const char *name);
    socket_error_t (*socket_open)(const char *name, socket_file_t **file);
    socket_error_t (*socket_read)(socket_file_t *file, void *buffer, uint32_t size, uint32_t *bytes_read);
    socket_error_t (*socket_write)(socket_file_t *file, const void *buffer, uint32_t size);
    socket_error_t (*socket_delete)(const char *name);
    socket_error_t (*socket_close)(socket_file_t *file);
    uint32_t (*socket_available)(socket_file_t *file);
    bool (*socket_exists)(const char *name);
} kernel_api_t;

#define log(fmt, level, vis, ...) g_api->log_internal(__FILE__, __LINE__, fmt, level, vis, ##__VA_ARGS__)
#define prints(str) g_api->prints(str)
#define printc(c) g_api->printc(c)
#define ft_run(set) g_api->ft_run(set)
#define setcolor(fg, bg) g_api->setcolor(fg, bg)
#define plotchar(c, x, y, fg) g_api->plotchar(c, x, y, fg)
#define draw_text_at(str, x, y, color) g_api->draw_text_at(str, x, y, color)
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
#define f_open(fs_select, filename, handle) g_api->f_open(fs_select, filename, handle)
#define f_read(fs_select, handle, buffer, size, br) g_api->f_read(fs_select, handle, buffer, size, br)
#define f_write(fs_select, handle, buffer, size) g_api->f_write(fs_select, handle, buffer, size)
#define f_close(fs_select, handle) g_api->f_close(fs_select, handle)
#define f_create(fs_select, filename, size) g_api->f_create(fs_select, filename, size)
#define f_rm(fs_select, filename) g_api->f_rm(fs_select, filename)
#define f_seek(fs_select, handle, position) g_api->f_seek(fs_select, handle, position)
#define f_format(drive) g_api->f_format(drive)
#define f_init(drive) g_api->f_init(drive)
#define f_mkdir(dirname) g_api->f_mkdir(dirname)
#define f_rmdir(dirname) g_api->f_rmdir(dirname)
#define f_chdir(dirname) g_api->f_chdir(dirname)
#define f_get_cwd(buffer, size) g_api->f_get_cwd(buffer, size)
#define f_list() g_api->f_list()
#define f_print_stats() g_api->f_print_stats()
#define f_unmount() g_api->f_unmount()
#define sleep(time) g_api->sleep(time)
#define sched_yield() g_api->sched_yield()
#define task_create(entry, name) g_api->task_create(entry, name)
#define task_create_user(entry, name) g_api->task_create_user(entry, name)
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
#define exec(filename, argc, argv) g_api->exec(filename, argc, argv)
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
#define socket_create(name) g_api->socket_create(name)
#define socket_open(name, file) g_api->socket_open(name, file)
#define socket_read(file, buffer, size, bytes_read) g_api->socket_read(file, buffer, size, bytes_read)
#define socket_write(file, buffer, size) g_api->socket_write(file, buffer, size)
#define socket_delete(name) g_api->socket_delete(name)
#define socket_close(file) g_api->socket_close(file)
#define socket_available(file) g_api->socket_available(file)
#define socket_exists(name) g_api->socket_exists(name)

#endif