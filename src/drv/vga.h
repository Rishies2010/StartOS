#ifndef VGA_H
#define VGA_H
#include <stdint.h>

extern int current_font;
extern uint64_t framebuffer_width, framebuffer_height;
extern uint8_t framebuffer_bpp;
extern uint8_t *framebuffer_addr;

void vga_init(void);
void clr(void);
void printc(char c);
void prints(const char *str);
void setcolor(uint8_t fg, uint8_t bg);
void plotlogo(uint32_t x_offset, uint32_t y_offset);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t get_pixel_at(uint32_t x, uint32_t y);
void draw_text_at(const char *text, uint32_t x, uint32_t y, uint32_t color);

#endif