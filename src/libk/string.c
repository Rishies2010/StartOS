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
    if (n == (size_t)-1) {
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
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

char* strrchr(const char* str, int c) {
    char* last = NULL;
    while (*str) {
        if (*str == c) {
            last = (char*)str;
        }
        str++;
    }
    return last;
}

void* memset(void* ptr, int value, size_t size) {
    unsigned char* p = (unsigned char*)ptr;
    while (size--) {
        *p++ = (unsigned char)value;
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
        // Copy forward
        while (size--) {
            *d++ = *s++;
        }
    } else {
        // Copy backward
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

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;}
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;}
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;}
    return result * sign;}

void itoa(int value, char *str)
{
    char *p = str;
    char *p1, *p2;
    unsigned int abs = (value < 0) ? -value : value;
    do
    {
        *p++ = '0' + (abs % 10);
        abs /= 10;
    } while (abs);
    if (value < 0)
        *p++ = '-';
    *p = '\0';
    p1 = str;
    p2 = p - 1;
    while (p1 < p2)
    {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}

int vsnprintf(char *str, size_t size, const char *format, va_list args) {
    size_t i = 0;
    const char *p = format;
    while (*p && i < size - 1) {
        if (*p == '%' && *(p + 1) == 's') {
            p += 2;
            const char *arg = va_arg(args, const char *);
            while (*arg && i < size - 1) {
                str[i++] = *arg++;
            }
        } else if (*p == '%' && *(p + 1) == 'd') {
            p += 2;
            int value = va_arg(args, int);
            char num_buffer[12];
            itoa(value, num_buffer);
            const char *arg = num_buffer;
            while (*arg && i < size - 1) {
                str[i++] = *arg++;
        }} else if (*p == '%' && *(p + 1) == 'u') {
            p += 2;
            unsigned int value = va_arg(args, unsigned int);
            char num_buffer[12];
            itoa(value, num_buffer);
            const char *arg = num_buffer;
            while (*arg && i < size - 1) {
                str[i++] = *arg++;
            }
        } else {
            str[i++] = *p++;
        }
    }
    str[i] = '\0';
    return i;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(str, size, format, args);
    va_end(args);
    return written;
}