#ifndef VGA_H
#define VGA_H
#include <stdint.h>

extern int current_font;
extern uint64_t framebuffer_width, framebuffer_height;

void vga_init(void);
void printc(char c);
void prints(const char* data, ...);
void clr(void);
void print_hex(uint32_t num);
void print_uint(uint32_t num);
void print_float(float num);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void setfont(int fontnum);
void setcolor(uint32_t color);
uint32_t makecolor(int r, int g, int b);
void draw_startlogo(uint32_t x_offset, uint32_t y_offset);

#endif