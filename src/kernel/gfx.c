#include "../drv/vga.h"
#include "../drv/keyboard.h"
#include "../drv/mouse.h"
#include "../drv/rtc.h"
#include "../libk/core/mem.h"
#include "gfx.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static window_t windows[MAX_WINDOWS];
static uint32_t window_count = 0;
static uint32_t focused_window = 0;
static uint32_t cursor_x = 0, cursor_y = 0;
static uint32_t next_window_id = 1;
static bool dragging = false;
static uint32_t drag_start_x, drag_start_y;
static uint32_t drag_window_x, drag_window_y;

void wm_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].id = 0;
        windows[i].visible = false;
        windows[i].focused = false;
        windows[i].buffer = NULL;
    }
    cursor_x = framebuffer_width / 2;
    cursor_y = framebuffer_height / 2;
}

void draw_xor_background(void) {
    for (uint32_t y = 0; y < framebuffer_height - TASKBAR_HEIGHT; y++) {
        for (uint32_t x = 0; x < framebuffer_width; x++) {
            uint32_t pattern = (x ^ y) & 0x3F;
            uint32_t color = makecolor(pattern, pattern + 20, pattern + 40);
            put_pixel(x, y, color);
        }
    }
}

void draw_cursor(void) {
    uint32_t white = makecolor(255, 255, 255);
    uint32_t black = makecolor(0, 0, 0);
    
    for (int i = 0; i < CURSOR_SIZE; i++) {
        put_pixel(cursor_x, cursor_y + i, white);
        put_pixel(cursor_x + 1, cursor_y + i, black);
        if (i < CURSOR_SIZE / 2) {
            put_pixel(cursor_x + i, cursor_y, white);
            put_pixel(cursor_x + i, cursor_y + 1, black);
        }
    }
}

void aa_put_pixel(uint32_t x, uint32_t y, uint32_t color, uint8_t alpha) {
    if (x >= framebuffer_width || y >= framebuffer_height) return;
    
    if (alpha == 255) {
        put_pixel(x, y, color);
        return;
    }
    
    uint32_t bg_r = 0, bg_g = 0, bg_b = 0;
    uint32_t fg_r = (color >> 16) & 0xFF;
    uint32_t fg_g = (color >> 8) & 0xFF;
    uint32_t fg_b = color & 0xFF;
    
    uint32_t final_r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    uint32_t final_g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    uint32_t final_b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;
    
    put_pixel(x, y, makecolor(final_r, final_g, final_b));
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_text_at(const char *text, uint32_t x, uint32_t y, uint32_t color) {
    uint32_t pos_x = x;
    while (*text) {
        draw_char(*text, pos_x, y);
        text++;
        pos_x += FONT_WIDTH;
    }
}

void draw_titlebar(window_t *win) {
    uint32_t title_color = win->focused ? makecolor(70, 130, 180) : makecolor(100, 100, 100);
    uint32_t text_color = makecolor(255, 255, 255);
    
    draw_rect(win->x, win->y, win->width, TITLEBAR_HEIGHT, title_color);
    draw_text_at(win->title, win->x + 5, win->y + 2, text_color);
    
    uint32_t close_x = win->x + win->width - 18;
    draw_rect(close_x, win->y + 2, 16, 16, makecolor(200, 50, 50));
    draw_text_at("X", close_x + 5, win->y + 2, text_color);
}

void draw_window(window_t *win) {
    if (!win->visible) return;
    
    draw_titlebar(win);
    
    uint32_t content_y = win->y + TITLEBAR_HEIGHT;
    uint32_t content_height = win->height - TITLEBAR_HEIGHT;
    
    for (uint32_t y = 0; y < content_height; y++) {
        for (uint32_t x = 0; x < win->width; x++) {
            if (content_y + y >= framebuffer_height - TASKBAR_HEIGHT) break;
            if (win->x + x >= framebuffer_width) break;
            
            uint32_t buf_idx = (y * win->width + x) * (framebuffer_bpp / 8);
            uint32_t color = *(uint32_t*)(win->buffer + buf_idx);
            put_pixel(win->x + x, content_y + y, color);
        }
    }
}

void draw_taskbar(void) {
    uint32_t taskbar_y = framebuffer_height - TASKBAR_HEIGHT;
    draw_rect(0, taskbar_y, framebuffer_width, TASKBAR_HEIGHT, makecolor(50, 50, 50));
    
    uint32_t x_pos = 5;
    for (uint32_t i = 0; i < window_count; i++) {
        if (!windows[i].visible) continue;
        
        uint32_t btn_color = windows[i].focused ? makecolor(100, 150, 200) : makecolor(80, 80, 80);
        draw_rect(x_pos, taskbar_y + 2, 120, 21, btn_color);
        
        char short_title[16];
        for (int j = 0; j < 15 && windows[i].title[j]; j++) {
            short_title[j] = windows[i].title[j];
        }
        short_title[15] = '\0';
        
        draw_text_at(short_title, x_pos + 3, taskbar_y + 5, makecolor(255, 255, 255));
        x_pos += 125;
        
        if (x_pos > framebuffer_width - 200) break;
    }
    
    rtc_time_t time = rtc_get_time();
    char time_str[16];
    time_str[0] = '0' + (time.hours / 10);
    time_str[1] = '0' + (time.hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (time.minutes / 10);
    time_str[4] = '0' + (time.minutes % 10);
    time_str[5] = '\0';
    
    draw_text_at(time_str, framebuffer_width - 60, taskbar_y + 5, makecolor(255, 255, 255));
}

uint32_t wm_create_window(const char *title, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (window_count >= MAX_WINDOWS) return 0;
    
    uint32_t idx = window_count++;
    windows[idx].id = next_window_id++;
    
    for (int i = 0; i < 63 && title[i]; i++) {
        windows[idx].title[i] = title[i];
    }
    windows[idx].title[63] = '\0';
    
    windows[idx].x = x;
    windows[idx].y = y;
    windows[idx].width = width;
    windows[idx].height = height;
    windows[idx].visible = true;
    windows[idx].focused = true;
    windows[idx].cursor_x = 0;
    windows[idx].cursor_y = 0;
    
    size_t buf_size = width * (height - TITLEBAR_HEIGHT) * (framebuffer_bpp / 8);
    windows[idx].buffer = (uint8_t*)kmalloc(buf_size);
    
    uint32_t bg_color = makecolor(240, 240, 240);
    for (size_t i = 0; i < buf_size; i += (framebuffer_bpp / 8)) {
        *(uint32_t*)(windows[idx].buffer + i) = bg_color;
    }
    
    for (uint32_t i = 0; i < window_count - 1; i++) {
        windows[i].focused = false;
    }
    focused_window = idx;
    
    return windows[idx].id;
}

void wm_destroy_window(uint32_t id) {
    for (uint32_t i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            if (windows[i].buffer) {
                kfree(windows[i].buffer);
            }
            
            for (uint32_t j = i; j < window_count - 1; j++) {
                windows[j] = windows[j + 1];
            }
            window_count--;
            
            if (focused_window >= window_count && window_count > 0) {
                focused_window = window_count - 1;
                windows[focused_window].focused = true;
            }
            break;
        }
    }
}

void wm_set_window_pos(uint32_t id, uint32_t x, uint32_t y) {
    for (uint32_t i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            windows[i].x = x;
            windows[i].y = y;
            break;
        }
    }
}

uint32_t wm_get_window_by_title(const char *title) {
    for (uint32_t i = 0; i < window_count; i++) {
        bool match = true;
        for (int j = 0; title[j] && windows[i].title[j]; j++) {
            if (title[j] != windows[i].title[j]) {
                match = false;
                break;
            }
        }
        if (match) return windows[i].id;
    }
    return 0;
}

void window_put_pixel(uint32_t id, uint32_t x, uint32_t y, uint32_t color) {
    for (uint32_t i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            uint32_t content_height = windows[i].height - TITLEBAR_HEIGHT;
            if (x >= windows[i].width || y >= content_height) return;
            
            uint32_t buf_idx = (y * windows[i].width + x) * (framebuffer_bpp / 8);
            *(uint32_t*)(windows[i].buffer + buf_idx) = color;
            break;
        }
    }
}

void window_printc(uint32_t id, char c, uint32_t x, uint32_t y) {
    for (uint32_t i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            uint32_t content_height = windows[i].height - TITLEBAR_HEIGHT;
            if (x + FONT_WIDTH > windows[i].width || y + FONT_HEIGHT > content_height) return;
            setcolor(makecolor(0, 0, 0));
            draw_char(c, x, y);
            break;
        }
    }
}

void window_prints(uint32_t id, const char *str, uint32_t x, uint32_t y) {
    uint32_t pos_x = x;
    while (*str) {
        window_printc(id, *str, pos_x, y);
        str++;
        pos_x += FONT_WIDTH;
    }
}

button_t wm_create_button(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char *text) {
    button_t btn;
    btn.x = x;
    btn.y = y;
    btn.width = width;
    btn.height = height;
    btn.pressed = false;
    
    for (int i = 0; i < 31 && text[i]; i++) {
        btn.text[i] = text[i];
    }
    btn.text[31] = '\0';
    
    return btn;
}

bool wm_button_clicked(button_t *btn, uint32_t mouse_x, uint32_t mouse_y, bool clicked) {
    bool hover = mouse_x >= btn->x && mouse_x < btn->x + btn->width &&
                 mouse_y >= btn->y && mouse_y < btn->y + btn->height;
    
    if (hover && clicked) {
        btn->pressed = true;
        return true;
    }
    
    btn->pressed = false;
    return false;
}

input_field_t wm_create_input(uint32_t x, uint32_t y, uint32_t width) {
    input_field_t input;
    input.x = x;
    input.y = y;
    input.width = width;
    input.text[0] = '\0';
    input.cursor_pos = 0;
    input.focused = false;
    return input;
}

void wm_handle_input_key(input_field_t *input, char key) {
    if (!input->focused) return;
    
    if (key == '\b' && input->cursor_pos > 0) {
        input->cursor_pos--;
        input->text[input->cursor_pos] = '\0';
    } else if (key >= 32 && key < 127 && input->cursor_pos < 63) {
        input->text[input->cursor_pos] = key;
        input->cursor_pos++;
        input->text[input->cursor_pos] = '\0';
    }
}

void wm_update(void) {
    draw_xor_background();
    
    for (uint32_t i = 0; i < window_count; i++) {
        draw_window(&windows[i]);
    }
    
    draw_taskbar();
    
    uint32_t new_x = mouse_x();
    uint32_t new_y = mouse_y();
    uint8_t mouse_btn = mouse_button();
    
    if (new_x != cursor_x || new_y != cursor_y) {
        cursor_x = new_x;
        cursor_y = new_y;
        
        if (cursor_x >= framebuffer_width) cursor_x = framebuffer_width - 1;
        if (cursor_y >= framebuffer_height) cursor_y = framebuffer_height - 1;
    }
    
    if (mouse_btn & 1) {
        if (!dragging) {
            for (uint32_t i = 0; i < window_count; i++) {
                if (cursor_x >= windows[i].x && cursor_x < windows[i].x + windows[i].width &&
                    cursor_y >= windows[i].y && cursor_y < windows[i].y + TITLEBAR_HEIGHT) {
                    
                    if (cursor_x >= windows[i].x + windows[i].width - 18) {
                        wm_destroy_window(windows[i].id);
                        break;
                    }
                    
                    for (uint32_t j = 0; j < window_count; j++) {
                        windows[j].focused = false;
                    }
                    windows[i].focused = true;
                    focused_window = i;
                    
                    dragging = true;
                    drag_start_x = cursor_x;
                    drag_start_y = cursor_y;
                    drag_window_x = windows[i].x;
                    drag_window_y = windows[i].y;
                    break;
                }
            }
        } else {
            int32_t dx = cursor_x - drag_start_x;
            int32_t dy = cursor_y - drag_start_y;
            windows[focused_window].x = drag_window_x + dx;
            windows[focused_window].y = drag_window_y + dy;
        }
    } else {
        dragging = false;
    }
    
    draw_cursor();
}