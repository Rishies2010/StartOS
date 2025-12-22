#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

unsigned char inportb(unsigned short port) {
    unsigned char rv;
    __asm__ __volatile__("inb %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}

void outportb(unsigned short port, unsigned char data) {
    __asm__ __volatile__("outb %1, %0" : : "dN" (port), "a" (data));
}

void serial_write_string(const char* str);

void serial_write_char(char c) {
    if(c == '\t') serial_write_string("    ");
    if(c == '\n') serial_write_char('\r');
    while (!(inportb(0x3F8 + 5) & 0x20));
    outportb(0x3F8, c);
}

void serial_write_string(const char* str) {
    while (*str) {
        serial_write_char(*str++);
    }
}

int main(int argc, char **argv)
{
    for(;;){
    serial_write_string("\x1b[38;2;50;255;50m[");
    serial_write_string(__FILE_NAME__);
    serial_write_string(":???]- Init Repeat\x1b[0m\n");}
    for(;;)asm volatile("hlt");
    return 0;
}