#include "core.h"
#include "../../libk/debug/log.h"
#include "../../drv/local_apic.h"
#include "../../kernel/sched.h"
#include "../../kernel/gfx.h"
#include "../../drv/rtc.h"
#include "../../drv/mouse.h"
#include "../../drv/keyboard.h"
#include "../../drv/vga.h"
#include "../../libk/gfx/startlogo.h"

void paint_main(void)
{
    uint32_t paint_win = wm_create_window("Paint App", 200, 150, 400, 300);

    for (uint32_t y = 0; y < 260; y++)
    {
        for (uint32_t x = 0; x < 380; x++)
        {
            window_put_pixel(paint_win, x, y, makecolor(255, 255, 255));
        }
    }

    window_prints(paint_win, "Click and drag to paint! R G B K C", 10, 10, makecolor(0, 0, 0));

    uint32_t last_mx = 0, last_my = 0;
    bool was_pressed = false;
    uint32_t brush_color = makecolor(0, 0, 255);
    wm_update();
    while (true)
    {
        if (mouse_moved())
        {

            char key = get_key();
            if (key == 27)
                break;
            if (key == 'r')
                brush_color = makecolor(255, 0, 0);
            if (key == 'g')
                brush_color = makecolor(0, 255, 0);
            if (key == 'b')
                brush_color = makecolor(0, 0, 255);
            if (key == 'k')
                brush_color = makecolor(0, 0, 0);
            if (key == 'c')
            {
                for (uint32_t y = 30; y < 260; y++)
                {
                    for (uint32_t x = 0; x < 380; x++)
                    {
                        window_put_pixel(paint_win, x, y, makecolor(255, 255, 255));
                    }
                }
            }
            uint32_t mx = mouse_x();
            uint32_t my = mouse_y();
            uint8_t mb = mouse_button();
            if (mb & 1)
            {
                optional_window optional_window_context = wm_get_window_context(paint_win);
                window_t window_context;
                if (optional_window_context.err == false)
                {
                    window_context = optional_window_context.window;
                }
                if (mx >= window_context.x && mx < window_context.x + window_context.width && my >= window_context.y && my < window_context.y + window_context.height - 20)
                {
                    uint32_t wx = mx - window_context.x;
                    uint32_t wy = my - (window_context.y + 20);

                    if (wy >= 30)
                    {
                        window_put_pixel(paint_win, wx, wy, brush_color);
                        window_put_pixel(paint_win, wx + 1, wy, brush_color);
                        window_put_pixel(paint_win, wx, wy + 1, brush_color);
                        window_put_pixel(paint_win, wx + 1, wy + 1, brush_color);

                        if (was_pressed)
                        {
                            int dx = (int)wx - (int)last_mx;
                            int dy = (int)wy - (int)last_my;
                            int steps = (dx > dy ? (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy));

                            /* If dx > dy {
                                If dx > -dx {
                                    steps = dx
                                }
                                    else
                                {
                                    steps = -dx
                                }
                            }

                            */
                            if (steps != 0)
                            {
                                for (int i = 0; i <= steps; i++)
                                {
                                    uint32_t ix = last_mx + (dx * i) / steps;
                                    uint32_t iy = last_my + (dy * i) / steps;
                                    if (iy >= 30)
                                    {
                                        window_put_pixel(paint_win, ix, iy, brush_color);
                                        window_put_pixel(paint_win, ix + 1, iy, brush_color);
                                        window_put_pixel(paint_win, ix, iy + 1, brush_color);
                                        window_put_pixel(paint_win, ix + 1, iy + 1, brush_color);
                                    }
                                }
                            }
                        }

                        last_mx = wx;
                        last_my = wy;
                        was_pressed = true;
                    }
                }
            }
            wm_update();
        }
        else
        {
            was_pressed = false;
        }
    }
}

void draw_startlogo(uint32_t x_offset, uint32_t y_offset, int id)
{
    unsigned int x, y;
    const char *data = header_data;
    unsigned char pixel[3];
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            HEADER_PIXEL(data, pixel);
            uint8_t r = 255 - pixel[0];
            uint8_t g = 255 - pixel[1];
            uint8_t b = 255 - pixel[2];
            uint32_t color = makecolor(r, g, b);
            window_put_pixel(id, x + x_offset, y + y_offset, color);
        }
    }
}

void ver_main()
{
    int ver = wm_create_window("Welcome", 20, 20, 180, 210);
    draw_startlogo(-10, 0, ver);
    window_prints(ver, "StartOS", 62, 4, makecolor(255, 255, 255));
}

void ap_main(void)
{
    __asm__ __volatile__("cli");
    if (LocalApicGetId() == 1)
    {
        __asm__ __volatile__("sti");
        wm_init();
        ver_main();
        paint_main();
        for (;;)
            ;
    }
    log("[CPU%i] Not participating. Halting.", 2, 0, LocalApicGetId());
    for (;;)
        __asm__ __volatile__("hlt");
}