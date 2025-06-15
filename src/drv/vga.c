#include "../drv/vga.h"
#include "../logic/log/logging.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;
uint8_t fgcol = VGA_COLOR_WHITE;
uint8_t bgcol = VGA_COLOR_BLACK;

// Create a VGA color byte
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}
// Create a VGA entry (character + color)
uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}
// Set the terminal's color.
void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}
// Hide the static blinking cursor.
void vga_hide_cursor(void) {
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, 0x20);
}
// Enable the cursor with a specified size (scanlines).
void vga_show_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, (inportb(0x3D5) & 0xC0) | cursor_start);
    outportb(0x3D4, 0x0B);
    outportb(0x3D5, (inportb(0x3D5) & 0xE0) | cursor_end);
}
// Update the hardware cursor position.
void vga_update_cursor(void) {
    uint16_t pos = terminal_row * 80 + terminal_column;
    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (uint8_t)(pos & 0xFF));
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
// Initialize the terminal.
void initterm(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(fgcol, bgcol);
    terminal_buffer = VGA_MEMORY;
    vga_show_cursor(15, 15);
    clr();
    log("VGA Text Mode driver Initialized", 1, 0);
}
// Print a character onto the screen
void printchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == 25) {
            for (size_t y = 0; y < (size_t)(25 - 1); y++) {
                for (size_t x = 0; x < 80; x++) {
                    terminal_buffer[y * 80 + x] = terminal_buffer[(y + 1) * 80 + x];}}
            for (size_t x = 0; x < 80; x++) {
                terminal_buffer[(25 - 1) * 80 + x] = vga_entry(' ', terminal_color);}
            terminal_row = 25 - 1;
        vga_update_cursor();}
        return;}
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_buffer[terminal_row * 80 + terminal_column] = vga_entry(' ', terminal_color);
        vga_update_cursor();}
        return;}
    terminal_buffer[terminal_row * 80 + terminal_column] = vga_entry(c, terminal_color);
    terminal_column++;
    if (terminal_column == 80) {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row == 25) {
            for (size_t y = 0; y < (size_t)(25 - 1); y++) {
                for (size_t x = 0; x < 80; x++) {
                    terminal_buffer[y * 80 + x] = terminal_buffer[(y + 1) * 80 + x];}}
            for (size_t x = 0; x < 80; x++) {
                terminal_buffer[(25 - 1) * 80 + x] = vga_entry(' ', terminal_color);}
            terminal_row = 25 - 1;}}
    vga_update_cursor();}
// Print a string that has $size characters printed.
void print(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printchar(data[i]);
    }
}
// Print a string with $size automatically detected.
void prints(const char* data) {
    print(data, strlen(data));
}
// Clear the Terminal screen.
void clr(void) {
    for (size_t y = 0; y < 25; y++) {
        for (size_t x = 0; x < 80; x++) {
            const size_t index = y * 80 + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);}}
    terminal_row = 0;
    terminal_column = 0;
}
// Show a simple boot screen as of now.
void boot(void) {
    clr();
    vga_hide_cursor();
    terminal_setcolor(vga_entry_color(11 , bgcol));
    prints("\n\n\n\n\n                                S T A R T    O S\n                             ______________________\n\n\n\n\n\n\n\n\n\n                                   ");
    terminal_setcolor(vga_entry_color(15 , bgcol));
    prints("BOOTING ");
    terminal_setcolor(vga_entry_color(12 , bgcol));
    prints("UP");
    clr();
    vga_update_cursor();
    terminal_setcolor(vga_entry_color(15, bgcol));
    vga_show_cursor(14, 15);
    log("Boot Animation Call Completed.", 4, 0);
}

// Print an unsigned integer to the screen
void print_uint(uint32_t num) {
    if (num == 0) {
        printchar('0');
        return;}
    uint32_t temp = num;
    int digits = 0;
    while (temp > 0) {
        digits++;
        temp /= 10;}
    char buffer[11];
    buffer[digits] = '\0';
    for (int i = digits - 1; i >= 0; i--) {
        buffer[i] = (num % 10) + '0';
        num /= 10;}
    prints(buffer);
}
// Print a value as hexadecimal to the screen
void print_hex(uint32_t num) {
    if (num == 0) {
        prints("0x0");
        return;}
    prints("0x");
    int digits = 0;
    uint32_t temp = num;
    while (temp > 0) {
        digits++;
        temp >>= 4;}
    char buffer[9];
    buffer[digits] = '\0';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;}
    prints(buffer);
}
void vga_draw_char(int x, int y, char c, uint8_t color) {
    if (x < 0 || x >= 80 || y < 0 || y >= 25) {
        log("Attempt to plot character beyond bounds.", 2, 1);
        return;
    }
    const size_t index = y * 80 + x;
    VGA_MEMORY[index] = vga_entry(c, color);
}
void drawstr(int x, int y, const char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        vga_draw_char(x + i, y, str[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
}