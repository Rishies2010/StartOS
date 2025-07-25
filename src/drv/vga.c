#include <stdint.h>
#include <stddef.h>
#include "../libk/core/mem.h"
#include "../libk/debug/log.h"
#include "../libk/string.h"
#include "../libk/limine.h"
#include "vga.h"
#include "../libk/gfx/font1_8x16.h"
#include "../libk/gfx/font2_8x16.h"
#include "../libk/gfx/font3_8x16.h"
#include "../libk/gfx/startlogo.h"
#include <stdbool.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = { 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, 0x9d5827dcd881dd75, 0xa3148604f6fab11b },
    .revision = 0
};

static struct limine_framebuffer *fb;
static uint8_t *framebuffer_addr;
uint64_t framebuffer_width, framebuffer_height, framebuffer_pitch;
static uint8_t framebuffer_bpp;
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint32_t terminal_color = 0xFFFFFF;
static const int CHAR_WIDTH = 8;
static const int CHAR_HEIGHT = 16;
int current_font = 2;
static size_t max_rows, max_cols;

void vga_init(void) {
    if (!framebuffer_request.response || !framebuffer_request.response->framebuffer_count) {
        log("[VGA] No framebuffer available", 3, 1);
        return;
    }
    
    fb = framebuffer_request.response->framebuffers[0];
    framebuffer_addr = (uint8_t *)fb->address;
    framebuffer_width = fb->width;
    framebuffer_height = fb->height;
    framebuffer_pitch = fb->pitch;
    framebuffer_bpp = fb->bpp;
    
    if (framebuffer_bpp != 16 && framebuffer_bpp != 24 && framebuffer_bpp != 32) {
        log("[VGA] Unsupported BPP: %i", 3, 1, framebuffer_bpp);
        return;
    }
    
    max_rows = framebuffer_height / CHAR_HEIGHT;
    max_cols = framebuffer_width / CHAR_WIDTH;
    current_font = 2;
    
    log("[VGA] Framebuffer initialized: %ix%i, %i bpp", 4, 0, 
        framebuffer_width, framebuffer_height, framebuffer_bpp);
}

void setfont(int fontnum) {
    if (fontnum >= 1 && fontnum <= 3) {
        current_font = fontnum;
    }
}

void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!framebuffer_addr || x >= framebuffer_width || y >= framebuffer_height) {
        return;
    }
    
    uint32_t offset = y * framebuffer_pitch + x * (framebuffer_bpp / 8);
    
    switch (framebuffer_bpp) {
        case 32:
            *(uint32_t*)(framebuffer_addr + offset) = color;
            break;
        case 24:
            framebuffer_addr[offset] = color & 0xFF;
            framebuffer_addr[offset + 1] = (color >> 8) & 0xFF;
            framebuffer_addr[offset + 2] = (color >> 16) & 0xFF;
            break;
        case 16: {
            uint16_t rgb565 = (((color >> 16) & 0xF8) << 8) | 
                             (((color >> 8) & 0xFC) << 3) | 
                             ((color & 0xF8) >> 3);
            *(uint16_t*)(framebuffer_addr + offset) = rgb565;
            break;
        }
    }
}

void draw_startlogo(uint32_t x_offset, uint32_t y_offset) {
    unsigned int x, y;
    const char *data = header_data;
    unsigned char pixel[3];
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            HEADER_PIXEL(data, pixel);
            uint8_t r = 255 - pixel[0];
            uint8_t g = 255 - pixel[1];
            uint8_t b = 255 - pixel[2];
            uint32_t color = makecolor(r, g, b);
            put_pixel(x + x_offset, y + y_offset, color);
        }
    }
}

void scroll_up(void) {
    if (!framebuffer_addr) return;
    
    size_t bytes_per_char_row = CHAR_HEIGHT * framebuffer_pitch;
    size_t total_scroll_bytes = bytes_per_char_row * (max_rows - 1);
    
    memmove(framebuffer_addr, framebuffer_addr + bytes_per_char_row, total_scroll_bytes);
    memset(framebuffer_addr + total_scroll_bytes, 0, bytes_per_char_row);
    if(!debug)draw_startlogo(framebuffer_width - 186, -16);
}

void setcolor(uint32_t color) {
    terminal_color = color;
}

uint32_t makecolor(int r, int g, int b) {
    return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

void newline(void) {
    terminal_column = 0;
    terminal_row++;
    if (terminal_row >= max_rows) {
        scroll_up();
        terminal_row = max_rows - 1;
    }
}

static void draw_char(char c, uint32_t x, uint32_t y) {
    const uint8_t *font_data;
    
    switch (current_font) {
        case 1: font_data = font1_8x16[(unsigned char)c]; break;
        case 3: font_data = font3_8x16[(unsigned char)c]; break;
        default: font_data = font2_8x16[(unsigned char)c]; break;
    }
    
    for (int row = 0; row < CHAR_HEIGHT; row++) {
        uint8_t font_row = font_data[row];
        for (int col = 0; col < CHAR_WIDTH; col++) {
            // uint32_t color = (font_row & (0x80 >> col)) ? terminal_color : 0x000000;
            if(font_row & (0x80 >> col))
            put_pixel(x + col, y + row, terminal_color);
        }
    }
}

void printc(char c) {
    switch (c) {
        case '\n':
            newline();
            return;
        case '\r':
            terminal_column = 0;
            return;
        case '\t':
            for (int i = 0; i < 4; i++) printc(' ');
            return;
        case '\b':
            if (terminal_column > 0) {
                terminal_column--;
                draw_char(' ', terminal_column * CHAR_WIDTH, terminal_row * CHAR_HEIGHT);
            }
            return;
    }
    
    if (terminal_column >= max_cols) {
        newline();
    }
    
    draw_char(c, terminal_column * CHAR_WIDTH, terminal_row * CHAR_HEIGHT);
    terminal_column++;
}

void prints(const char* data, ...) {
    if (!data) return;
    
    va_list args;
    va_start(args, data);
    
    size_t len = strlen(data);
    size_t buffer_size = len * 2 + 256;
    char *buffer = (char*)kmalloc(buffer_size);
    
    if (!buffer) {
        va_end(args);
        return;
    }
    
    int result = vsnprintf(buffer, buffer_size, data, args);
    va_end(args);
    
    if (result > 0) {
        for (int i = 0; i < result && buffer[i]; i++) {
            printc(buffer[i]);
        }
    }
    
    kfree(buffer);
}

void clr(void) {
    if (!framebuffer_addr) return;
    
    size_t total_bytes = framebuffer_height * framebuffer_pitch;
    memset(framebuffer_addr, 0, total_bytes);
    terminal_row = 0;
    terminal_column = 0;
}

void print_uint(uint32_t num) {
    if (num == 0) {
        printc('0');
        return;
    }
    
    char buffer[12];
    int pos = 0;
    
    while (num > 0) {
        buffer[pos++] = (num % 10) + '0';
        num /= 10;
    }
    
    for (int i = pos - 1; i >= 0; i--) {
        printc(buffer[i]);
    }
}

void print_hex(uint32_t num) {
    prints("0x");
    if (num == 0) {
        printc('0');
        return;
    }
    
    char buffer[9];
    int pos = 0;
    const char hex_chars[] = "0123456789ABCDEF";
    
    while (num > 0) {
        buffer[pos++] = hex_chars[num & 0xF];
        num >>= 4;
    }
    
    for (int i = pos - 1; i >= 0; i--) {
        printc(buffer[i]);
    }
}

void print_float(float num) {
    if (num < 0) {
        printc('-');
        num = -num;
    }
    
    uint32_t integer_part = (uint32_t)num;
    print_uint(integer_part);
    printc('.');
    
    float fractional_part = num - (float)integer_part;
    for (int i = 0; i < 3; i++) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        printc('0' + digit);
        fractional_part -= digit;
    }
}