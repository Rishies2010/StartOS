#ifndef USERLIB_H
#define USERLIB_H

typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

static inline uint64_t syscall0(uint64_t num) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t syscall1(uint64_t num, uint64_t arg1) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(arg1) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t syscall2(uint64_t num, uint64_t arg1, uint64_t arg2) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(arg1), "S"(arg2) : "rcx", "r11", "memory");
    return ret;
}

static inline void exit(int code) {
    (void)code;
    syscall0(1);
    while(1);
}

static inline char getkey(void) {
    return (char)syscall0(2);
}

static inline void prints(const char *str) {
    if (!str) return;
    uint32_t len = 0;
    while (str[len]) len++;
    syscall2(3, (uint64_t)str, len);
}

static inline uint32_t mouse_x(void) {
    return (uint32_t)syscall0(4);
}

static inline uint32_t mouse_y(void) {
    return (uint32_t)syscall0(5);
}

static inline uint8_t mouse_button(void) {
    return (uint8_t)syscall0(6);
}

static inline void speaker_play(uint32_t hz) {
    syscall1(7, hz);
}

static inline void speaker_stop(void) {
    syscall0(8);
}

#endif