#include "gui.h"
#include <stddef.h>

static kernel_api_t *g_api = NULL;
static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static dirty_rect_t dirty_area = {0};
static cursor_t cursor = {0};
static window_t *active_window = NULL;
static bool run = false;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define TITLEBAR_HEIGHT 24

static inline void expand_dirty(int x1, int y1, int x2, int y2)
{
    if (!dirty_area.active)
    {
        dirty_area.x1 = x1;
        dirty_area.y1 = y1;
        dirty_area.x2 = x2;
        dirty_area.y2 = y2;
        dirty_area.active = true;
    }
    else
    {
        if (x1 < dirty_area.x1)
            dirty_area.x1 = x1;
        if (y1 < dirty_area.y1)
            dirty_area.y1 = y1;
        if (x2 > dirty_area.x2)
            dirty_area.x2 = x2;
        if (y2 > dirty_area.y2)
            dirty_area.y2 = y2;
    }
}

void mark_dirty(int x1, int y1, int x2, int y2)
{
    expand_dirty(x1, y1, x2, y2);
}

void clear_dirty(void)
{
    dirty_area.active = false;
}

void wm_init(void)
{

    asm volatile("cli");
    ft_run(false);
    for (int i = 0; i < MAX_WINDOWS; i++)
    {
        windows[i].visible = false;
        windows[i].buffer = NULL;
    }
    window_count = 0;

    cursor.x = SCREEN_WIDTH / 2;
    cursor.y = SCREEN_HEIGHT / 2;
    cursor.prev_x = -1;
    cursor.prev_y = -1;
    cursor.needs_restore = false;
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            put_pixel(x, y, GUI_BG_COLOR);
        }
    }
    run = true;
    asm volatile("sti");
}

window_t *create_window(int x, int y, int width, int height, const char *title)
{
    if (window_count >= MAX_WINDOWS)
    {
        log("Max windows reached", 2, 0);
        return NULL;
    }

    window_t *win = &windows[window_count++];
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->visible = true;
    win->dragging = false;
    win->dirty = true;

    int i = 0;
    while (title[i] != '\0' && i < 63)
    {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';

    win->buffer = (uint32_t *)kmalloc(width * height * sizeof(uint32_t));
    if (!win->buffer)
    {
        log("Failed to allocate window buffer", 3, 0);
        window_count--;
        return NULL;
    }

    for (int i = 0; i < width * height; i++)
    {
        win->buffer[i] = GUI_WINDOW_BG;
    }

    return win;
}

void destroy_window(window_t *win)
{
    if (!win || !win->visible)
        return;

    if (win->buffer)
    {
        kfree(win->buffer);
        win->buffer = NULL;
    }

    win->visible = false;

    mark_dirty(win->x, win->y, win->x + win->width, win->y + win->height + TITLEBAR_HEIGHT);
}

void draw_window(window_t *win)
{
    if (!win || !win->visible)
        return;

    asm volatile("cli");

    int wx = win->x;
    int wy = win->y;
    int ww = win->width;
    int wh = win->height;

    for (int y = 4; y < wh + TITLEBAR_HEIGHT + 4; y++)
    {
        for (int x = 4; x < ww + 4; x++)
        {
            int sx = wx + x;
            int sy = wy + y;
            if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT)
            {
                put_pixel(sx, sy, GUI_SHADOW);
            }
        }
    }

    for (int y = 0; y < TITLEBAR_HEIGHT; y++)
    {
        for (int x = 0; x < ww; x++)
        {
            int px = wx + x;
            int py = wy + y;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
            {
                put_pixel(px, py, GUI_TITLEBAR_BG);
            }
        }
    }

    int tx = wx + 8;
    int ty = wy + 6;
    for (int i = 0; win->title[i] != '\0'; i++)
    {
        if (tx + 8 >= wx + ww)
            break;
        plotchar(win->title[i], tx, ty, GUI_TITLEBAR_TEXT);
        tx += 8;
    }

    for (int x = 0; x < ww; x++)
    {
        int px = wx + x;
        int py = wy + TITLEBAR_HEIGHT;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
        {
            put_pixel(px, py, GUI_WINDOW_BORDER);
        }
    }

    for (int y = 0; y < wh; y++)
    {
        for (int x = 0; x < ww; x++)
        {
            int px = wx + x;
            int py = wy + TITLEBAR_HEIGHT + 1 + y;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
            {
                uint32_t color = win->buffer[y * ww + x];
                put_pixel(px, py, color);
            }
        }
    }

    for (int y = 0; y <= wh; y++)
    {
        int py = wy + TITLEBAR_HEIGHT + y;
        if (py >= 0 && py < SCREEN_HEIGHT)
        {
            if (wx >= 0 && wx < SCREEN_WIDTH)
                put_pixel(wx, py, GUI_WINDOW_BORDER);
            if (wx + ww - 1 >= 0 && wx + ww - 1 < SCREEN_WIDTH)
                put_pixel(wx + ww - 1, py, GUI_WINDOW_BORDER);
        }
    }

    for (int x = 0; x < ww; x++)
    {
        int px = wx + x;
        int py = wy + TITLEBAR_HEIGHT + wh;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
        {
            put_pixel(px, py, GUI_WINDOW_BORDER);
        }
    }

    asm volatile("sti");

    win->dirty = false;
}

void set_dirty(window_t *win)
{
    if (!win)
        return;
    win->dirty = true;
    mark_dirty(win->x, win->y,
                   win->x + win->width,
                   win->y + win->height + TITLEBAR_HEIGHT);
}

void win_clear(window_t *win, uint32_t color)
{
    if (!win || !win->buffer)
        return;

    for (int i = 0; i < win->width * win->height; i++)
    {
        win->buffer[i] = color;
    }
    set_dirty(win);
}

void win_pixel(window_t *win, int x, int y, uint32_t color)
{
    if (!win || !win->buffer)
        return;
    if (x < 0 || x >= win->width || y < 0 || y >= win->height)
        return;

    win->buffer[y * win->width + x] = color;
    set_dirty(win);
}

void restore_cursor(void)
{
    if (!cursor.needs_restore)
        return;

    asm volatile("cli");
    for (int y = 0; y < CURSOR_SIZE; y++)
    {
        for (int x = 0; x < CURSOR_SIZE; x++)
        {
            int px = cursor.prev_x + x;
            int py = cursor.prev_y + y;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
            {
                put_pixel(px, py, cursor.save_buffer[y * CURSOR_SIZE + x]);
            }
        }
    }
    asm volatile("sti");

    cursor.needs_restore = false;
}

void draw_cursor(void)
{

    asm volatile("cli");
    for (int y = 0; y < CURSOR_SIZE; y++)
    {
        for (int x = 0; x < CURSOR_SIZE; x++)
        {
            int px = cursor.x + x;
            int py = cursor.y + y;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
            {
                cursor.save_buffer[y * CURSOR_SIZE + x] = get_pixel_at(px, py);
            }
        }
    }

    static const char cursor_shape[CURSOR_SIZE][CURSOR_SIZE] = {
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
        {1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0},
        {1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0},
        {1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0},
        {1, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0},
        {1, 2, 2, 1, 2, 1, 0, 0, 0, 0, 0, 0},
        {1, 2, 1, 0, 1, 2, 1, 0, 0, 0, 0, 0},
        {1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}};

    for (int y = 0; y < CURSOR_SIZE; y++)
    {
        for (int x = 0; x < CURSOR_SIZE; x++)
        {
            int px = cursor.x + x;
            int py = cursor.y + y;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
            {
                if (cursor_shape[y][x] == 1)
                {
                    put_pixel(px, py, 0x000000);
                }
                else if (cursor_shape[y][x] == 2)
                {
                    put_pixel(px, py, GUI_CURSOR_COLOR);
                }
            }
        }
    }
    asm volatile("sti");

    cursor.needs_restore = true;
    cursor.prev_x = cursor.x;
    cursor.prev_y = cursor.y;
}

void update_cursor_pos(void)
{
    uint32_t new_x = mouse_x();
    uint32_t new_y = mouse_y();

    if (new_x != cursor.x || new_y != cursor.y)
    {
        restore_cursor();
        cursor.x = new_x;
        cursor.y = new_y;
        draw_cursor();
    }
}

void wm_update(void)
{

    if (mouse_moved())
    {
        update_cursor_pos();
    }

    uint8_t btn = mouse_button();

    for (int i = 0; i < window_count; i++)
    {
        window_t *win = &windows[i];
        if (!win->visible)
            continue;

        if (btn == 1)
        {
            int mx = cursor.x;
            int my = cursor.y;

            if (!win->dragging &&
                mx >= win->x && mx < win->x + win->width &&
                my >= win->y && my < win->y + TITLEBAR_HEIGHT)
            {
                win->dragging = true;
                win->drag_offset_x = mx - win->x;
                win->drag_offset_y = my - win->y;
                active_window = win;
            }

            if (win->dragging)
            {
                int new_x = cursor.x - win->drag_offset_x;
                int new_y = cursor.y - win->drag_offset_y;

                if (new_x != win->x || new_y != win->y)
                {

                    mark_dirty(win->x, win->y,
                                   win->x + win->width + 4,
                                   win->y + win->height + TITLEBAR_HEIGHT + 4);

                    win->x = new_x;
                    win->y = new_y;
                    win->dirty = true;
                }
            }
        }
        else
        {
            win->dragging = false;
        }
    }

    if (dirty_area.active)
    {
        restore_cursor();

        asm volatile("cli");
        for (int y = dirty_area.y1; y <= dirty_area.y2 && y < SCREEN_HEIGHT; y++)
        {
            for (int x = dirty_area.x1; x <= dirty_area.x2 && x < SCREEN_WIDTH; x++)
            {
                if (x >= 0 && y >= 0)
                {
                    put_pixel(x, y, GUI_BG_COLOR);
                }
            }
        }
        asm volatile("sti");

        for (int i = 0; i < window_count; i++)
        {
            if (windows[i].visible)
            {
                draw_window(&windows[i]);
            }
        }

        clear_dirty();
        draw_cursor();
    }
    else
    {

        for (int i = 0; i < window_count; i++)
        {
            if (windows[i].visible && windows[i].dirty)
            {
                restore_cursor();
                draw_window(&windows[i]);
                draw_cursor();
            }
        }
    }

    sched_yield();
}

void wm_cleanup(void)
{
    asm volatile("cli");
    for (int i = 0; i < window_count; i++)
    {
        if (windows[i].buffer)
        {
            kfree(windows[i].buffer);
            windows[i].buffer = NULL;
        }
    }
    ft_run(true);
    run = false;
    asm volatile("sti");
}

int main(int argc, char **argv, kernel_api_t *api)
{
    g_api = api;
    asm volatile("cli");
    wm_init();

    window_t *win1 = create_window(100, 100, 300, 200, "Demo Window");
    if (win1)
    {
        win_clear(win1, 0x00665E);
    }

    window_t *win2 = create_window(150, 150, 250, 150, "Another Window");
    if (win2)
    {
        win_clear(win2, 0xFFFFFF);
    }

    while (run)
    {
        wm_update();
        char key = get_key();
        if (key == 'Q')
        {
            run = false;
        }
        sched_yield();
    }
    wm_cleanup();
    asm volatile("sti");
    return 0;
}