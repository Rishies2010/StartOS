#ifndef VGA_H
#define VGA_H
#include <stdint.h>

void vga_init(void);
void printc(char c);
void prints(const char* data, ...);
void clr(void);
void print_hex(uint32_t num);
void print_uint(uint32_t num);
void print_float(float num);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void setcolor(uint32_t color);
uint32_t makecolor(int r, int g, int b);

#endif