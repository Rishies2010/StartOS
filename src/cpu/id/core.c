#include "core.h"
#include "../../libk/debug/log.h"
#include "../../drv/local_apic.h"
#include "../../kernel/sched.h"
#include "../../kernel/gfx.h"
#include "../../drv/rtc.h"
#include "../../drv/mouse.h"
#include "../../drv/keyboard.h"
#include "../../drv/vga.h"

void paint_main(void) {
    wm_init();
    
    uint32_t paint_win = wm_create_window("Paint App", 200, 150, 400, 300);
    
    for (uint32_t y = 0; y < 260; y++) {
        for (uint32_t x = 0; x < 380; x++) {
            window_put_pixel(paint_win, x, y, makecolor(255, 255, 255));
        }
    }
    
    window_prints(paint_win, "Click and drag to paint!", 10, 10);
    
    uint32_t last_mx = 0, last_my = 0;
    bool was_pressed = false;
    uint32_t brush_color = makecolor(0, 0, 255);
    
    while (true) {
        if(mouse_moved()){
        wm_update();
        
        char key = get_key();
        if (key == 27) break;
        
        if (key == 'r') brush_color = makecolor(255, 0, 0);
        if (key == 'g') brush_color = makecolor(0, 255, 0);
        if (key == 'b') brush_color = makecolor(0, 0, 255);
        if (key == 'k') brush_color = makecolor(0, 0, 0);
        if (key == 'c') {
            for (uint32_t y = 30; y < 260; y++) {
                for (uint32_t x = 0; x < 380; x++) {
                    window_put_pixel(paint_win, x, y, makecolor(255, 255, 255));
                }
            }
        }
        
        uint32_t mx = mouse_x();
        uint32_t my = mouse_y();
        uint8_t mb = mouse_button();
        
        if (mb & 1) {
            if (mx >= 200 && mx < 580 && my >= 180 && my < 430) {
                uint32_t wx = mx - 200;
                uint32_t wy = my - 180;
                
                if (wy >= 30) {
                    window_put_pixel(paint_win, wx, wy, brush_color);
                    window_put_pixel(paint_win, wx+1, wy, brush_color);
                    window_put_pixel(paint_win, wx, wy+1, brush_color);
                    window_put_pixel(paint_win, wx+1, wy+1, brush_color);
                    
                    if (was_pressed) {
                        int dx = (int)wx - (int)last_mx;
                        int dy = (int)wy - (int)last_my;
                        int steps = (dx > dy ? (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy));
                        
                        for (int i = 0; i <= steps; i++) {
                            uint32_t ix = last_mx + (dx * i) / steps;
                            uint32_t iy = last_my + (dy * i) / steps;
                            if (iy >= 30) {
                                window_put_pixel(paint_win, ix, iy, brush_color);
                                window_put_pixel(paint_win, ix+1, iy, brush_color);
                                window_put_pixel(paint_win, ix, iy+1, brush_color);
                                window_put_pixel(paint_win, ix+1, iy+1, brush_color);
                            }
                        }
                    }
                    
                    last_mx = wx;
                    last_my = wy;
                    was_pressed = true;
                }
            }}
        } else {
            was_pressed = false;
        }
    }
}

void ap_main(void){
    __asm__ __volatile__("cli");
    if(LocalApicGetId() == 1){
        paint_main();
        for(;;);
    }
    log("[CPU%i] Not participating. Halting.", 2, 0, LocalApicGetId());
    for(;;)__asm__ __volatile__("hlt");
}