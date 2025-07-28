#include "core.h"
#include "../../libk/debug/log.h"
#include "../../drv/local_apic.h"
#include "../../drv/vga.h"
#include "../../kernel/sched.h"
#include "../../drv/mouse.h"
#include "../../libk/ports.h"

int cursor_old_x = 0;
int cursor_old_y = 0;
int outline = 0x202020;
int fill = 0x808080;

void draw_cursor() {
    int size = 20;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size - y; x++) {
            put_pixel(cursor_old_x + x, cursor_old_y + y, 0x000000);
        }
    }
    
    int x = mouse_x();
    int y = mouse_y();
    uint8_t buttons = mouse_button();
    int current_outline = outline;
    int current_fill = fill;
    
    if (buttons & 0x01) current_fill = 0xFFFFFF;
    if (buttons & 0x02) {
        outportw(0x604, 0x2000);
        outportw(0xB004, 0x2000);
        outportw(0x4004, 0x3400);
        outportb(0xB2, 0x0F);
        outportw(0x8900, 0x2001);
        outportw(0x8900, 0xFF00);
        __asm__ __volatile__("cli");
        __asm__ __volatile__("hlt");
    }
    if (buttons & 0x04) current_fill = 0xFFFFFF;
    
    for (int py = 0; py < size; py++) {
        for (int px = 0; px < size - py; px++) {
            if (px == 0 || px == size - py - 1 || py == 0) {
                put_pixel(x + px, y + py, current_outline);
            } else {
                put_pixel(x + px, y + py, current_fill);
            }
        }
    }
    cursor_old_x = x;
    cursor_old_y = y;
}

void cursor(void) {
    if (mouse_moved()) {
        draw_cursor();
    }
}

void ap_main(void){
    __asm__ __volatile__("cli");
    if(LocalApicGetId() == 1){
        add_task(cursor, "Mouse Cursor");
        cursor_old_x = framebuffer_width / 2;
        cursor_old_y = framebuffer_height / 2;
        schedule();
        for(;;)__asm__ __volatile__("hlt");
    }
    log("[CPU%i] Not participating. Halting.", 2, 0, LocalApicGetId());
    for(;;)__asm__ __volatile__("hlt");
}