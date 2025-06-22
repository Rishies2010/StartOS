#include "string.h"
#include <stddef.h>
#include <stdarg.h>

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* start = dest;
    while ((*dest++ = *src++));
    return start;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* start = dest;
    while (n && (*dest++ = *src++)) {
        n--;
    }
    while (n--) {
        *dest++ = '\0';
    }
    return start;
}

char* strcat(char* dest, const char* src) {
    char* start = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return start;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* start = dest;
    while (*dest) {
        dest++;
    }
    while (n-- && (*dest++ = *src++));
    if (n == SIZE_MAX) {
        *dest = '\0';
    }
    return start;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strncmp(const char* str1, const char* str2, size_t n) {
    while (n && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    if ((char)c == '\0') {
        return (char*)str;
    }
    return NULL;
}

char* strrchr(const char* str, int c) {
    char* last = NULL;
    do {
        if (*str == (char)c) {
            last = (char*)str;
        }
    } while (*str++);
    return last;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
        haystack++;
    }
    return NULL;
}

void* memset(void* ptr, int value, size_t size) {
    unsigned char* p = (unsigned char*)ptr;
    unsigned char val = (unsigned char)value;
    while (size--) {
        *p++ = val;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t size) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (size--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t size) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        while (size--) {
            *d++ = *s++;
        }
    } else {
        d += size;
        s += size;
        while (size--) {
            *--d = *--s;
        }
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t size) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    while (size--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void* memchr(const void* ptr, int value, size_t size) {
    const unsigned char* p = (const unsigned char*)ptr;
    unsigned char val = (unsigned char)value;
    
    while (size--) {
        if (*p == val) {
            return (void*)p;
        }
        p++;
    }
    return NULL;
}

size_t strspn(const char* str1, const char* str2) {
    size_t len = 0;
    const char* p;
    
    while (*str1) {
        p = str2;
        while (*p) {
            if (*str1 == *p) break;
            p++;
        }
        if (!*p) break;
        str1++;
        len++;
    }
    return len;
}

size_t strcspn(const char* str1, const char* str2) {
    size_t len = 0;
    const char* p;
    
    while (*str1) {
        p = str2;
        while (*p) {
            if (*str1 == *p) return len;
            p++;
        }
        str1++;
        len++;
    }
    return len;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {
        i++;
    }
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

void itoa(int value, char* str) {
    char* p = str;
    char* p1, *p2;
    unsigned int abs_val = (value < 0) ? -value : value;
    do {
        *p++ = '0' + (abs_val % 10);
        abs_val /= 10;
    } while (abs_val);
    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    p1 = str;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}

void itoa_hex(unsigned long value, char* str) {
    const char hex_chars[] = "0123456789abcdef";
    char temp[20];
    int i = 0;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    while (value > 0) {
        temp[i++] = hex_chars[value % 16];
        value /= 16;
    }
    int j = 0;
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

static int format_int(char* buf, size_t buf_size, long long value, int base, int uppercase, int width, int zero_pad) {
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[32];
    int temp_len = 0;
    int negative = 0;
    unsigned long long abs_val;
    
    if (value < 0 && base == 10) {
        negative = 1;
        abs_val = -value;
    } else {
        abs_val = value;
    }
    if (abs_val == 0) {
        temp[temp_len++] = '0';
    } else {
        while (abs_val > 0) {
            temp[temp_len++] = digits[abs_val % base];
            abs_val /= base;
        }
    }
    int total_len = temp_len;
    if (negative) total_len++;
    int pad_len = (width > total_len) ? width - total_len : 0;
    int written = 0;
    if (!zero_pad && pad_len > 0) {
        for (int i = 0; i < pad_len && written < buf_size - 1; i++) {
            buf[written++] = ' ';
        }
    }
    if (negative && written < buf_size - 1) {
        buf[written++] = '-';
    }
    if (zero_pad && pad_len > 0) {
        for (int i = 0; i < pad_len && written < buf_size - 1; i++) {
            buf[written++] = '0';
        }
    }
    for (int i = temp_len - 1; i >= 0 && written < buf_size - 1; i--) {
        buf[written++] = temp[i];
    }
    
    return written;
}

int vsnprintf(char* str, size_t size, const char* format, va_list args) {
    size_t written = 0;
    const char* p = format;
    
    if (size == 0) return 0;
    
    while (*p && written < size - 1) {
        if (*p != '%') {
            str[written++] = *p++;
            continue;
        }
        
        p++;
        int zero_pad = 0;
        int left_align = 0;
        
        while (*p == '0' || *p == '-') {
            if (*p == '0') zero_pad = 1;
            if (*p == '-') left_align = 1;
            p++;
        }
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }
        int is_long = 0;
        int is_long_long = 0;
        if (*p == 'l') {
            is_long = 1;
            p++;
            if (*p == 'l') {
                is_long_long = 1;
                p++;
            }
        }
        switch (*p) {
            case 's': {
                const char* arg = va_arg(args, const char*);
                if (!arg) arg = "(null)";
                while (*arg && written < size - 1) {
                    str[written++] = *arg++;
                }
                break;
            }
            case 'c': {
                int c = va_arg(args, int);
                if (written < size - 1) {
                    str[written++] = (char)c;
                }
                break;
            }
            case 'd':
            case 'i': {
                long long value;
                if (is_long_long) {
                    value = va_arg(args, long long);
                } else if (is_long) {
                    value = va_arg(args, long);
                } else {
                    value = va_arg(args, int);
                }
                char temp_buf[32];
                int len = format_int(temp_buf, sizeof(temp_buf), value, 10, 0, width, zero_pad);
                for (int i = 0; i < len && written < size - 1; i++) {
                    str[written++] = temp_buf[i];
                }
                break;
            }
            case 'u': {
                unsigned long long value;
                if (is_long_long) {
                    value = va_arg(args, unsigned long long);
                } else if (is_long) {
                    value = va_arg(args, unsigned long);
                } else {
                    value = va_arg(args, unsigned int);
                }
                char temp_buf[32];
                int len = format_int(temp_buf, sizeof(temp_buf), value, 10, 0, width, zero_pad);
                for (int i = 0; i < len && written < size - 1; i++) {
                    str[written++] = temp_buf[i];
                }
                break;
            }
            case 'x': {
                unsigned long long value;
                if (is_long_long) {
                    value = va_arg(args, unsigned long long);
                } else if (is_long) {
                    value = va_arg(args, unsigned long);
                } else {
                    value = va_arg(args, unsigned int);
                }
                char temp_buf[32];
                int len = format_int(temp_buf, sizeof(temp_buf), value, 16, 0, width, zero_pad);
                for (int i = 0; i < len && written < size - 1; i++) {
                    str[written++] = temp_buf[i];
                }
                break;
            }
            case 'X': {
                unsigned long long value;
                if (is_long_long) {
                    value = va_arg(args, unsigned long long);
                } else if (is_long) {
                    value = va_arg(args, unsigned long);
                } else {
                    value = va_arg(args, unsigned int);
                }
                char temp_buf[32];
                int len = format_int(temp_buf, sizeof(temp_buf), value, 16, 1, width, zero_pad);
                for (int i = 0; i < len && written < size - 1; i++) {
                    str[written++] = temp_buf[i];
                }
                break;
            }
            case 'p': {
                void* ptr = va_arg(args, void*);
                unsigned long long addr = (unsigned long long)ptr;
                if (written < size - 3) {
                    str[written++] = '0';
                    str[written++] = 'x';
                }
                char temp_buf[32];
                int len = format_int(temp_buf, sizeof(temp_buf), addr, 16, 0, 0, 0);
                for (int i = 0; i < len && written < size - 1; i++) {
                    str[written++] = temp_buf[i];
                }
                break;
            }
            case '%':
                if (written < size - 1) {
                    str[written++] = '%';
                }
                break;
            default:
                if (written < size - 2) {
                    str[written++] = '%';
                    str[written++] = *p;
                }
                break;
        }
        p++;
    }
    
    str[written] = '\0';
    return written;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(str, size, format, args);
    va_end(args);
    return written;
}