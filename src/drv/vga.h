#ifndef VGA_H
#define VGA_H
#include <stddef.h>
#include <stdint.h>
#include "../libk/ports.h"
#include "../libk/string.h"
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15};
extern uint8_t fgcol;
extern uint8_t bgcol;
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);
uint16_t vga_entry(unsigned char c, uint8_t color);
void vga_show_cursor(uint8_t cursor_start, uint8_t cursor_end);
void initterm(void);
void terminal_setcolor(uint8_t color);
void printchar(char c);
void print(const char* data, size_t size);
void prints(const char* data);
void clr(void);
void boot(void);
void vga_hide_cursor(void);
void print_hex(uint32_t num);
void print_uint(uint32_t num);
void vga_update_cursor(void);
void vga_draw_char(int x, int y, char c, uint8_t color);
void drawstr(int x, int y, const char* str);
#endif