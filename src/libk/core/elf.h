#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include "../../drv/rtc.h"
#include "../../drv/disk/sfs.h"

#define ELF_MAGIC 0x464C457F
#define ELF_CLASS_64 2
#define ET_EXEC 2
#define EM_X86_64 62
#define PT_LOAD 1
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf64_ehdr_t;

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

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

int elf_exec(const char *filename);

#endif