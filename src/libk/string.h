#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String length
size_t strlen(const char* str);

// String copy
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

// String concatenation
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);

// String comparison
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);

// Character search
char* strchr(const char* str, int c);
char* strrchr(const char* str, int c);

// Memory functions
void* memset(void* ptr, int value, size_t size);
void* memcpy(void* dest, const void* src, size_t size);
void* memmove(void* dest, const void* src, size_t size);
int memcmp(const void* ptr1, const void* ptr2, size_t size);

#endif // STRING_H