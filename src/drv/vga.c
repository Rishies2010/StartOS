#include <stdint.h>
#include <stddef.h>
#include "../libk/core/mem.h"
#include "../libk/debug/log.h"
#include "../libk/string.h"
#include "../libk/limine.h"
#include "vga.h"
#include "../libk/gfx/font_8x16.h"
#include <stdbool.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0};

static struct limine_framebuffer *fb;
static uint32_t *framebuffer_addr;
static uint64_t framebuffer_width, framebuffer_height, framebuffer_pitch;
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint32_t terminal_color = 0xFFFFFF;
static const int CHAR_WIDTH = 8;
static const int CHAR_HEIGHT = 16;
static size_t max_rows, max_cols;

void vga_init(void){
    fb = framebuffer_request.response->framebuffers[0];
    framebuffer_addr = (uint32_t *)fb->address;
    framebuffer_width = fb->width;
    framebuffer_height = fb->height;
    framebuffer_pitch = fb->pitch;
    max_rows = framebuffer_height / CHAR_HEIGHT;
    max_cols = framebuffer_width / CHAR_WIDTH;
    log("Framebuffer found and initialized. Height : %i; Width : %i.", 4, 0, framebuffer_height, framebuffer_width);
}

void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (framebuffer_addr && x < framebuffer_width && y < framebuffer_height) {
        uint32_t* fb = (uint32_t*)framebuffer_addr;
        fb[y * (framebuffer_pitch / 4) + x] = color;
    }
}

void scroll_up(void) {
    uint32_t* fb = (uint32_t*)framebuffer_addr;
    size_t pixels_per_row = framebuffer_pitch / 4;
    size_t char_rows_to_move = max_rows - 1;
    size_t pixels_to_move = char_rows_to_move * CHAR_HEIGHT * pixels_per_row;
    
    memmove(fb, fb + (CHAR_HEIGHT * pixels_per_row), pixels_to_move * sizeof(uint32_t));
    
    size_t last_row_start = (max_rows - 1) * CHAR_HEIGHT * pixels_per_row;
    size_t last_row_pixels = CHAR_HEIGHT * pixels_per_row;
    memset(fb + last_row_start, 0, last_row_pixels * sizeof(uint32_t));
}

void setcolor(uint32_t color) {
    terminal_color = color;
}

uint32_t makecolor(int r, int g, int b) {
    return (r << 16) | (g << 8) | b;
}

void newline(void) {
    terminal_column = 0;
    terminal_row++;
    if (terminal_row >= max_rows) {
        scroll_up();
        terminal_row = max_rows - 1;
    }
}

void printc(char c) {
    if (c == '\n') {
        newline();
        return;
    }
    
    if (c == '\r') {
        terminal_column = 0;
        return;
    }
    
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            uint32_t start_x = terminal_column * CHAR_WIDTH;
            uint32_t start_y = terminal_row * CHAR_HEIGHT;
            
            for (int row = 0; row < CHAR_HEIGHT; row++) {
                for (int col = 0; col < CHAR_WIDTH; col++) {
                    put_pixel(start_x + col, start_y + row, 0x000000);
                }
            }
        }
        return;
    }
    
    if (terminal_column >= max_cols) {
        newline();
    }
    
    uint32_t start_x = terminal_column * CHAR_WIDTH;
    uint32_t start_y = terminal_row * CHAR_HEIGHT;
    
    for (int row = 0; row < CHAR_HEIGHT; row++) {
        uint8_t font_row = font_8x16[(unsigned char)c][row];
        for (int col = 0; col < CHAR_WIDTH; col++) {
            if (font_row & (0x80 >> col)) {
                put_pixel(start_x + col, start_y + row, terminal_color);
            } else {
                put_pixel(start_x + col, start_y + row, 0x000000);
            }
        }
    }
    
    terminal_column++;
}

void prints(const char* data, ...) {
    va_list args;
    va_start(args, data);
    char temp[512];
    unsigned long len = vsnprintf(temp, sizeof(temp), data, args);
    va_end(args);
    if (len < 0) return;
    if (len < sizeof(temp)) {
        for (unsigned long i = 0; i < len; i++) {
            printc(temp[i]);
        }
    } else {
        char* buffer = (char*)kmalloc(len + 1);
        if (!buffer) return;
        va_start(args, data);
        vsnprintf(buffer, len + 1, data, args);
        va_end(args);
        for (unsigned long i = 0; i < len; i++) {
            printc(buffer[i]);
        }
        kfree(buffer);
    }
}

void clr(void) {
    if (!framebuffer_addr) return;
    uint32_t* fb = (uint32_t*)framebuffer_addr;
    size_t total_pixels = (framebuffer_height * framebuffer_pitch) / 4;
    for (size_t i = 0; i < total_pixels; i++) {
        fb[i] = 0x000000;
    }
    terminal_row = 0;
    terminal_column = 0;
}

void print_uint(uint32_t num) {
    if (num == 0) {
        printc('0');
        return;
    }
    uint32_t temp = num;
    int digits = 0;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }
    char buffer[11];
    buffer[digits] = '\0';
    for (int i = digits - 1; i >= 0; i--) {
        buffer[i] = (num % 10) + '0';
        num /= 10;
    }
    prints(buffer);
}

void print_hex(uint32_t num) {
    if (num == 0) {
        prints("0x0");
        return;
    }
    prints("0x");
    int digits = 0;
    uint32_t temp = num;
    while (temp > 0) {
        digits++;
        temp >>= 4;
    }
    char buffer[9];
    buffer[digits] = '\0';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    prints(buffer);
}