#ifndef VGA_H
#define VGA_H
#include <stddef.h>
#include <stdint.h>
#include "../libk/ports.h"
#include "../libk/string.h"

extern uint64_t framebuffer_addr;
extern uint32_t framebuffer_width;
extern uint32_t framebuffer_height; 
extern uint32_t framebuffer_pitch;
extern uint8_t framebuffer_bpp;
void printc(char c);
void prints(const char* data, ...);
void clr(void);
void print_hex(uint32_t num);
void print_uint(uint32_t num);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void setcolor(uint32_t color);
uint32_t makecolor(int r, int g, int b);
#endif