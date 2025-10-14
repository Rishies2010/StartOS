#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../kernel_api.h"

static kernel_api_t *g_api = NULL;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define MAX_COLS (SCREEN_WIDTH / CHAR_WIDTH)
#define MAX_ROWS ((SCREEN_HEIGHT - 24) / CHAR_HEIGHT)
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 256

#define BG_COLOR 0x1a1b26
#define TEXT_COLOR 0xc0caf5
#define CURSOR_COLOR 0x7aa2f7
#define STATUS_BG 0x24283b
#define STATUS_TEXT 0x9ece6a
#define LINE_NUM_COLOR 0x565f89

typedef struct
{
    char text[MAX_LINE_LENGTH];
    int length;
} line_t;

static line_t lines[MAX_LINES];
static int total_lines = 1;
static int cursor_line = 0;
static int cursor_col = 0;
static int scroll_offset = 0;
static bool cursor_visible = true;
static uint32_t last_blink = 0;
static bool modified = false;
static bool needs_full_redraw = true;
static int prev_cursor_line = -1;
static int prev_cursor_col = -1;

typedef struct
{
    int x1, y1, x2, y2;
    bool active;
} dirty_rect_t;

static dirty_rect_t dirty_area = {0};

void mark_dirty_area(int x1, int y1, int x2, int y2)
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

void clear_dirty_area()
{
    dirty_area.active = false;
}

void clear_rect(int x1, int y1, int x2, int y2)
{
    for (int y = y1; y <= y2; y++)
    {
        for (int x = x1; x <= x2; x++)
        {
            if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
            {
                put_pixel(x, y, BG_COLOR);
            }
        }
    }
}

void clear_screen()
{
    clear_rect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}

void draw_status_bar() {
    for (int y = SCREEN_HEIGHT - 24; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            put_pixel(x, y, STATUS_BG);
        }
    }
    
    char status[128];
    char *ptr = status;
    
    *ptr++ = ' ';
    *ptr++ = 'L';
    *ptr++ = 'i';
    *ptr++ = 'n';
    *ptr++ = 'e';
    *ptr++ = ' ';
    
    int line = cursor_line + 1;
    if (line >= 100) *ptr++ = '0' + (line / 100);
    if (line >= 10) *ptr++ = '0' + ((line / 10) % 10);
    *ptr++ = '0' + (line % 10);
    *ptr++ = '/';
    
    if (total_lines >= 100) *ptr++ = '0' + (total_lines / 100);
    if (total_lines >= 10) *ptr++ = '0' + ((total_lines / 10) % 10);
    *ptr++ = '0' + (total_lines % 10);
    
    *ptr++ = ' ';
    *ptr++ = ' ';
    *ptr++ = 'C';
    *ptr++ = 'o';
    *ptr++ = 'l';
    *ptr++ = ' ';
    
    int col = cursor_col + 1;
    if (col >= 100) *ptr++ = '0' + (col / 100);
    if (col >= 10) *ptr++ = '0' + ((col / 10) % 10);
    *ptr++ = '0' + (col % 10);
    
    if (modified) {
        *ptr++ = ' ';
        *ptr++ = ' ';
        *ptr++ = '[';
        *ptr++ = 'm';
        *ptr++ = 'o';
        *ptr++ = 'd';
        *ptr++ = ']';
    }
    *ptr = '\0';
    
    int x_px = 8;
    int y_px = SCREEN_HEIGHT - 16;
    for (int i = 0; status[i] != '\0'; i++) {
        plotchar(status[i], x_px, y_px, STATUS_TEXT, STATUS_BG);
        x_px += CHAR_WIDTH;
    }
    
    const char* help = "Ctrl+S=Save  Ctrl+Q=Quit";
    x_px = SCREEN_WIDTH - (25 * CHAR_WIDTH);
    for (int i = 0; help[i] != '\0'; i++) {
        plotchar(help[i], x_px, y_px, LINE_NUM_COLOR, STATUS_BG);
        x_px += CHAR_WIDTH;
    }
}

void draw_line_numbers()
{
    int text_start_x = CHAR_WIDTH * 5;

    for (int i = 0; i < MAX_ROWS && (scroll_offset + i) < total_lines; i++)
    {
        int line_num = scroll_offset + i + 1;

        int line_y = i * CHAR_HEIGHT + 4;
        if (!needs_full_redraw &&
            (line_y + CHAR_HEIGHT < dirty_area.y1 || line_y > dirty_area.y2))
        {
            continue;
        }

        char num[4];
        int idx = 0;

        if (line_num >= 100)
            num[idx++] = '0' + (line_num / 100);
        else
            num[idx++] = ' ';

        if (line_num >= 10)
            num[idx++] = '0' + ((line_num / 10) % 10);
        else
            num[idx++] = ' ';

        num[idx++] = '0' + (line_num % 10);
        num[idx] = '\0';

        int x_px = 4;
        int y_px = line_y;

        clear_rect(0, y_px, text_start_x - 1, y_px + CHAR_HEIGHT - 1);

        for (int j = 0; num[j] != '\0'; j++)
        {
            plotchar(num[j], x_px, y_px, LINE_NUM_COLOR, BG_COLOR);
            x_px += CHAR_WIDTH;
        }
    }
}

void render_text_area()
{
    int text_start_x = CHAR_WIDTH * 5;

    for (int i = 0; i < MAX_ROWS && (scroll_offset + i) < total_lines; i++)
    {
        int line_idx = scroll_offset + i;
        int x_px = text_start_x;
        int y_px = i * CHAR_HEIGHT + 4;

        if (!needs_full_redraw &&
            (y_px + CHAR_HEIGHT < dirty_area.y1 || y_px > dirty_area.y2))
        {
            continue;
        }

        clear_rect(text_start_x, y_px, SCREEN_WIDTH - 1, y_px + CHAR_HEIGHT - 1);

        for (int j = 0; j < lines[line_idx].length; j++)
        {
            if (x_px >= SCREEN_WIDTH - CHAR_WIDTH)
                break;
            plotchar(lines[line_idx].text[j], x_px, y_px, TEXT_COLOR, BG_COLOR);
            x_px += CHAR_WIDTH;
        }
    }
}

void render_cursor()
{
    int text_start_x = CHAR_WIDTH * 5;

    if (prev_cursor_line >= 0 && prev_cursor_col >= 0 &&
        (prev_cursor_line != cursor_line || prev_cursor_col != cursor_col))
    {

        int old_line_idx = prev_cursor_line - scroll_offset;
        if (old_line_idx >= 0 && old_line_idx < MAX_ROWS)
        {
            int old_y = old_line_idx * CHAR_HEIGHT + 4;
            int old_x = text_start_x + (prev_cursor_col * CHAR_WIDTH);

            if (old_x < SCREEN_WIDTH - CHAR_WIDTH)
            {

                int text_line_idx = prev_cursor_line;
                if (prev_cursor_col < lines[text_line_idx].length)
                {
                    plotchar(lines[text_line_idx].text[prev_cursor_col],
                             old_x, old_y, TEXT_COLOR, BG_COLOR);
                }
                else
                {
                    clear_rect(old_x, old_y, old_x + 1, old_y + CHAR_HEIGHT - 1);
                }
            }
        }
    }

    if (cursor_visible)
    {
        int line_idx = cursor_line - scroll_offset;
        if (line_idx >= 0 && line_idx < MAX_ROWS)
        {
            int cursor_y = line_idx * CHAR_HEIGHT + 4;
            int cursor_x = text_start_x + (cursor_col * CHAR_WIDTH);

            if (cursor_x < SCREEN_WIDTH - CHAR_WIDTH)
            {
                for (int cy = 0; cy < CHAR_HEIGHT; cy++)
                {
                    for (int cx = 0; cx < 2; cx++)
                    {
                        put_pixel(cursor_x + cx, cursor_y + cy, CURSOR_COLOR);
                    }
                }
            }
        }
    }

    prev_cursor_line = cursor_line;
    prev_cursor_col = cursor_col;
}

void render()
{
    if (needs_full_redraw)
    {
        clear_screen();
        render_text_area();
        draw_line_numbers();
        draw_status_bar();
        render_cursor();
        needs_full_redraw = false;
        clear_dirty_area();
    }
    else if (dirty_area.active)
    {

        render_text_area();
        draw_line_numbers();
        draw_status_bar();
        render_cursor();
        clear_dirty_area();
    }
    else
    {

        render_cursor();
    }
}

void mark_full_redraw()
{
    needs_full_redraw = true;
}

void mark_line_dirty(int line_idx)
{
    if (line_idx >= scroll_offset && line_idx < scroll_offset + MAX_ROWS)
    {
        int screen_line = line_idx - scroll_offset;
        int y1 = screen_line * CHAR_HEIGHT + 4;
        int y2 = y1 + CHAR_HEIGHT - 1;
        mark_dirty_area(0, y1, SCREEN_WIDTH - 1, y2);
    }
}

void insert_char(char c)
{
    if (lines[cursor_line].length < MAX_LINE_LENGTH - 1)
    {
        for (int i = lines[cursor_line].length; i > cursor_col; i--)
        {
            lines[cursor_line].text[i] = lines[cursor_line].text[i - 1];
        }
        lines[cursor_line].text[cursor_col] = c;
        lines[cursor_line].length++;
        cursor_col++;
        modified = true;
        mark_line_dirty(cursor_line);
    }
}

void handle_backspace()
{
    if (cursor_col > 0)
    {
        for (int i = cursor_col - 1; i < lines[cursor_line].length - 1; i++)
        {
            lines[cursor_line].text[i] = lines[cursor_line].text[i + 1];
        }
        lines[cursor_line].length--;
        cursor_col--;
        modified = true;
        mark_line_dirty(cursor_line);
    }
    else if (cursor_line > 0)
    {
        int prev_len = lines[cursor_line - 1].length;
        if (prev_len + lines[cursor_line].length < MAX_LINE_LENGTH)
        {

            for (int i = 0; i < lines[cursor_line].length; i++)
            {
                lines[cursor_line - 1].text[prev_len + i] = lines[cursor_line].text[i];
            }
            lines[cursor_line - 1].length += lines[cursor_line].length;

            for (int i = cursor_line; i < total_lines - 1; i++)
            {
                lines[i] = lines[i + 1];
            }
            total_lines--;

            lines[total_lines].length = 0;

            cursor_line--;
            cursor_col = prev_len;
            modified = true;
            mark_full_redraw();
        }
    }
}

void handle_enter()
{
    if (total_lines >= MAX_LINES)
        return;

    for (int i = total_lines; i > cursor_line + 1; i--)
    {
        lines[i] = lines[i - 1];
    }

    int split_len = lines[cursor_line].length - cursor_col;
    for (int i = 0; i < split_len; i++)
    {
        lines[cursor_line + 1].text[i] = lines[cursor_line].text[cursor_col + i];
    }
    lines[cursor_line + 1].length = split_len;
    lines[cursor_line].length = cursor_col;

    total_lines++;
    cursor_line++;
    cursor_col = 0;
    modified = true;

    if (cursor_line >= scroll_offset + MAX_ROWS)
    {
        scroll_offset++;
    }

    mark_full_redraw();
}

void save_file()
{
    sfs_file_t file;
    sfs_error_t err;

    err = f_create("notes.txt", 8192);
    if (err != SFS_OK && err != SFS_ERR_ALREADY_EXISTS)
    {
        log("Failed to create file", 2, 0);
        return;
    }

    err = f_open("notes.txt", &file);
    if (err != SFS_OK)
    {
        log("Failed to open file", 3, 0);
        return;
    }

    for (int i = 0; i < total_lines; i++)
    {
        if (lines[i].length > 0)
        {
            f_write(&file, lines[i].text, lines[i].length);
        }
        if (i < total_lines - 1)
        {
            f_write(&file, "\n", 1);
        }
    }

    f_close(&file);
    modified = false;

    mark_dirty_area(0, SCREEN_HEIGHT - 24, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}

void load_file()
{
    sfs_file_t file;
    sfs_error_t err = f_open("notes.txt", &file);

    for (int i = 0; i < MAX_LINES; i++)
    {
        lines[i].length = 0;
    }
    total_lines = 1;

    if (err == SFS_OK)
    {
        char buffer[1024];
        uint32_t bytes_read = 0;

        err = f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read);
        if (err == SFS_OK || err == SFS_ERR_EOF)
        {
            buffer[bytes_read] = '\0';

            total_lines = 0;
            int line_idx = 0;
            int char_idx = 0;

            for (uint32_t i = 0; i < bytes_read && line_idx < MAX_LINES; i++)
            {
                if (buffer[i] == '\n')
                {
                    lines[line_idx].length = char_idx;
                    line_idx++;
                    char_idx = 0;
                    if (line_idx >= MAX_LINES)
                        break;
                }
                else if (char_idx < MAX_LINE_LENGTH - 1)
                {
                    lines[line_idx].text[char_idx++] = buffer[i];
                }
            }

            if (char_idx > 0 && line_idx < MAX_LINES)
            {
                lines[line_idx].length = char_idx;
                line_idx++;
            }

            total_lines = (line_idx > 0) ? line_idx : 1;
            modified = false;
        }

        f_close(&file);
    }

    cursor_line = 0;
    cursor_col = 0;
    scroll_offset = 0;
    prev_cursor_line = -1;
    prev_cursor_col = -1;
}

int main(kernel_api_t *api)
{
    g_api = api;
    asm volatile("cli");
    if (!g_api)
        return -1;

    ft_run(false);

    for (int i = 0; i < MAX_LINES; i++)
    {
        lines[i].length = 0;
    }
    total_lines = 1;

    load_file();

    rtc_time_t boot = rtc_boottime();
    last_blink = boot.seconds;

    mark_full_redraw();
    render();

    while (true)
    {
        rtc_time_t current = rtc_get_time();

        if (current.seconds != last_blink)
        {
            cursor_visible = !cursor_visible;
            last_blink = current.seconds;
            render();
        }

        char key = get_key();
        if (key != '\0')
        {
            cursor_visible = true;
            bool handled = false;

            if (is_ctrl_pressed())
            {
                if (key == 'q' || key == 'Q')
                {
                    break;
                }
                else if (key == 's' || key == 'S')
                {
                    save_file();
                    mark_full_redraw();
                    render();
                    continue;
                }
            }

            switch (key)
            {
            case ']':
                if (cursor_col < lines[cursor_line].length)
                {
                    cursor_col++;
                    render();
                }
                handled = true;
                break;

            case '[':
                if (cursor_col > 0)
                {
                    cursor_col--;
                    render();
                }
                handled = true;
                break;

            case '-':
                if (cursor_line > 0)
                {
                    cursor_line--;
                    if (cursor_col > lines[cursor_line].length)
                    {
                        cursor_col = lines[cursor_line].length;
                    }
                    if (cursor_line < scroll_offset)
                    {
                        scroll_offset--;
                        mark_full_redraw();
                    }
                    render();
                }
                handled = true;
                break;

            case '=':
                if (cursor_line < total_lines - 1)
                {
                    cursor_line++;
                    if (cursor_col > lines[cursor_line].length)
                    {
                        cursor_col = lines[cursor_line].length;
                    }
                    if (cursor_line >= scroll_offset + MAX_ROWS)
                    {
                        scroll_offset++;
                        mark_full_redraw();
                    }
                    render();
                }
                handled = true;
                break;

            case '\n':
                handle_enter();
                mark_full_redraw();
                render();
                handled = true;
                break;

            case '\b':
                handle_backspace();
                render();
                handled = true;
                break;

            default:
                if (key >= 32 && key < 127)
                {
                    insert_char(key);
                    render();
                }
                break;
            }
        }

        sched_yield();
    }

    ft_run(true);
    asm volatile("sti");
    return 0;
}