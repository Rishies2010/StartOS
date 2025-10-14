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
    size_t (*strlen)(const char* str);
    char* (*strcpy)(char* dest, const char* src);
    char* (*strcat)(char* dest, const char* src);
    int (*strcmp)(const char* str1, const char* str2);
    int (*atoi)(const char* str);
    void (*itoa)(int value, char* str);
    void (*itoa_hex)(unsigned long value, char* str);
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
#define f_format(drive) api->f_format(drive)
#define f_init(drive) api->f_init(drive)
#define f_mkdir(dirname) api->f_mkdir(dirname)
#define f_rmdir(dirname) api->f_rmdir(dirname)
#define f_chdir(dirname) api->f_chdir(dirname)
#define f_get_cwd(buffer, size) api->f_get_cwd(buffer, size)
#define f_create(filename, size) api->f_create(filename, size)
#define f_open(filename, file) api->f_open(filename, file)
#define f_read(file, buffer, size, br) api->f_read(file, buffer, size, br)
#define f_write(file, buffer, size) api->f_write(file, buffer, size)
#define f_close(file) api->f_close(file)
#define f_rm(filename) api->f_rm(filename)
#define f_mk(filename, size) api->f_mk(filename, size)
#define f_seek(file, position) api->f_seek(file, position)
#define f_list() api->f_list()
#define f_print_stats() api->f_print_stats()
#define f_unmount() api->f_unmount()
#define sleep(time) api->sleep(time)
#define sched_yield() api->sched_yield()
#define strlen(str) api->strlen(str)
#define strcpy(dest, src) api->strcpy(dest, src)
#define strcat(dest, src) api->strcat(dest, src)
#define strcmp(str1, str2) api->strcmp(str1, str2)
#define atoi(str) api->atoi(str)
#define itoa(value, str) api->itoa(value, str)
#define itoa_hex(value, str) api->itoa_hex(value, str)
#define get_pixel_at(x, y) api->get_pixel_at(x, y)
#define exec(filename) api->exec(filename)
#define void mouse_init(void);
#define mouse_x(void) api->mouse_x(void)
#define mouse_y(void) api->mouse_y(void)
#define mouse_button(void) api->mouse_button(void)
#define mouse_moved(void) api->mouse_moved(void)
#define clr(void) api->clr(void)
#define speaker_note(octave, note) api->speaker_note(octave, note)
#define speaker_play(hz) api->speaker_play(hz)
#define speaker_pause(void) api->speaker_pause(void)
#define rtc_get_time(void) api->rtc_get_time(void)
#define rtc_calculate_uptime(start, end) api->rtc_calculate_uptime(start, end)
#define rtc_boottime(void) api->rtc_boottime(void)
#define net_send_packet(data, len) api->net_send_packet(data, len)
#define net_receive_packet(buf, buf_size) api->net_receive_packet(buf, buf_size)
#define net_get_mac_address(mac) api->net_get_mac_address(mac)
#define net_link_up() api->net_link_up()
#define net_enable_interrupts() api->net_enable_interrupts()
#define net_disable_interrupts() api->net_disable_interrupts()
#define net_get_interrupt_status() api->net_get_interrupt_status()
#define net_handle_interrupt() api->net_handle_interrupt()

#endif