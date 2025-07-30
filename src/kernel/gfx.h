#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_WINDOWS 16
#define TITLEBAR_HEIGHT 20
#define TASKBAR_HEIGHT 25
#define CURSOR_SIZE 12
#define FONT_WIDTH 8
#define FONT_HEIGHT 16

typedef struct {
    uint32_t id;
    char title[64];
    uint32_t x, y, width, height;
    uint8_t *buffer;
    bool state;
    bool focused;
    uint32_t cursor_x, cursor_y;
} window_t;

typedef struct
{
    bool err;
    window_t window;
} optional_window;

typedef struct {
    uint32_t x, y, width, height;
    char text[32];
    bool pressed;
} button_t;

typedef struct {
    uint32_t x, y, width;
    char text[64];
    uint32_t cursor_pos;
    bool focused;
} input_field_t;

void wm_init(void);
void wm_update(void);

uint32_t wm_create_window(const char *title, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void wm_destroy_window(uint32_t id);
void wm_set_window_pos(uint32_t id, uint32_t x, uint32_t y);
uint32_t wm_get_window_by_title(const char *title);
optional_window wm_get_window_context(const uint32_t id);

void window_put_pixel(uint32_t id, uint32_t x, uint32_t y, uint32_t color);
void window_printc(uint32_t id, char c, uint32_t x, uint32_t y, uint32_t color);
void window_prints(uint32_t id, const char *str, uint32_t x, uint32_t y, uint32_t color);

button_t wm_create_button(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char *text);
bool wm_button_clicked(button_t *btn, uint32_t mouse_x, uint32_t mouse_y, bool clicked);

input_field_t wm_create_input(uint32_t x, uint32_t y, uint32_t width);
void wm_handle_input_key(input_field_t *input, char key);

void draw_xor_background(void);
void draw_cursor(void);
void aa_put_pixel(uint32_t x, uint32_t y, uint32_t color, uint8_t alpha);
void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void draw_text_at(const char *text, uint32_t x, uint32_t y, uint32_t color);

#endif