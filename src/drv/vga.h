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

#endif