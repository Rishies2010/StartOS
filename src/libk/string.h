#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdarg.h>

// Standard string functions
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strchr(const char* str, int c);
char* strrchr(const char* str, int c);
size_t strspn(const char* str1, const char* str2);
size_t strcspn(const char* str1, const char* str2);
char* strstr(const char* haystack, const char* needle);

// Standard memory functions
void* memset(void* ptr, int value, size_t size);
void* memcpy(void* dest, const void* src, size_t size);
void* memmove(void* dest, const void* src, size_t size);
int memcmp(const void* ptr1, const void* ptr2, size_t size);
void* memchr(const void* ptr, int value, size_t size);

// Utility functions (non-standard but useful for kernel dev)
int atoi(const char* str);
void itoa(int value, char* str);
void itoa_hex(unsigned long value, char* str);

// Printf-style formatting functions
int vsnprintf(char* str, size_t size, const char* format, va_list args);
int snprintf(char* str, size_t size, const char* format, ...);

//Case changers
char toLower(char c);
char toUpper(char c);

#endif