#include <stdint.h>
#include <stddef.h>
#include "../libk/core/mem.h"
#include "../libk/debug/log.h"
#include "../libk/string.h"
#include "../libk/spinlock.h"
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
uint8_t framebuffer_bpp;
static uint32_t terminal_color = 0x000000;
static int term_col = 0, term_row = 0;
static int max_rows, max_cols;
int current_font = 2;
static spinlock_t vga_lock;

void vga_init(void) {
    spinlock_init(&vga_lock);
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
    max_rows = framebuffer_height/16; 
    max_cols = framebuffer_width/8;
    
    if (framebuffer_bpp != 16 && framebuffer_bpp != 24 && framebuffer_bpp != 32) {
        log("[VGA] Unsupported BPP: %i", 3, 1, framebuffer_bpp);
        return;
    }
    
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
    if (x >= framebuffer_width || y >= framebuffer_height) return;
    
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

void setcolor(uint32_t color) {
    terminal_color = color;
}

uint32_t makecolor(int r, int g, int b) {
    return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

void draw_char(char c, uint32_t x, uint32_t y) {
    const uint8_t *font_data;
    
    switch (current_font) {
        case 1: font_data = font1_8x16[(unsigned char)c]; break;
        case 3: font_data = font3_8x16[(unsigned char)c]; break;
        default: font_data = font2_8x16[(unsigned char)c]; break;
    }
    
    for (int row = 0; row < 16; row++) {
        uint8_t font_row = font_data[row];
        for (int col = 0; col < 8; col++) {
            if(font_row & (0x80 >> col))
                put_pixel(x + col, y + row, terminal_color);
        }
    }
}

void clr(void) {
    spinlock_acquire(&vga_lock);
    if (!framebuffer_addr) {
        spinlock_release(&vga_lock);
        return;
    }
    
    size_t total_bytes = framebuffer_height * framebuffer_pitch;
    memset(framebuffer_addr, 0, total_bytes);
    term_col = 0;
    term_row = 0;
    spinlock_release(&vga_lock);
}

void scroll(void) {
    size_t bytes_per_line = framebuffer_pitch;
    size_t scroll_size = (framebuffer_height - 16) * bytes_per_line;
    
    memmove(framebuffer_addr, framebuffer_addr + 16 * bytes_per_line, scroll_size);
    memset(framebuffer_addr + scroll_size, 0, 16 * bytes_per_line);
}

void printc(char c) {
    spinlock_acquire(&vga_lock);
    
    if (c == '\n') {
        term_col = 0;
        term_row++;
        if (term_row >= max_rows) {
            scroll();
            term_row = max_rows - 1;
        }
        spinlock_release(&vga_lock);
        return;
    }
    
    draw_char(c, term_col * 8, term_row * 16);
    term_col++;
    
    if (term_col >= max_cols) {
        term_col = 0;
        term_row++;
        if (term_row >= max_rows) {
            scroll();
            term_row = max_rows - 1;
        }
    }
    
    spinlock_release(&vga_lock);
}

void prints(const char *str) {
    while (*str) {
        printc(*str++);
    }
    return;
}

void printi(uint64_t num) {
    char buffer[21];
    bool is_negative = false;
    size_t i = 0;
    
    if (num == 0) {
        printc('0');
        return;
    }
    
    if ((int64_t)num < 0) {
        is_negative = true;
        num = -(int64_t)num;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    while (i > 0) {
        printc(buffer[--i]);
    }
}

void printh(uint64_t num) {
    prints("0x");
    char hex_digits[] = "0123456789ABCDEF";
    char buffer[17];
    int i = 0;
    
    if (num == 0) {
        printc('0');
        return;
    }
    
    while (num > 0) {
        buffer[i++] = hex_digits[num % 16];
        num /= 16;
    }
    
    while (i > 0) {
        printc(buffer[--i]);
    }
}