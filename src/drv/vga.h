#ifndef VGA_H
#define VGA_H
#include <stdint.h>

extern int current_font;
extern uint64_t framebuffer_width, framebuffer_height;
extern uint8_t framebuffer_bpp;

void vga_init(void);
void clr(void);
void printc(char c);
void prints(const char *str);
void printh(uint64_t num);
void printi(uint64_t num);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void setfont(int fontnum);
void setcolor(uint32_t color);
uint32_t makecolor(int r, int g, int b);
void draw_startlogo(uint32_t x_offset, uint32_t y_offset);
void draw_char(char c, uint32_t x, uint32_t y);

#endif