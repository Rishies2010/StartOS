#include "../drv/vga.h"
#include "gfx.h"

#define M_PI		3.14159265358979323846
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ABS(x) ((x)<0?-(x):(x))
#define CLAMP(x,low,high) (((x)>(high))?(high):(((x)<(low))?(low):(x)))

static float mix(float a, float b, float t) { return a + (b - a) * t; }
static float smoothstep(float e0, float e1, float x) {
    x = CLAMP((x - e0) / (e1 - e0), 0.0, 1.0);
    return x * x * (3 - 2 * x);
}

static float sqrt(float x) {
    if (x <= 0) return 0;
    float guess = x;
    for (int i = 0; i < 10; i++) {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

static float sin(float x) {
    x = x - (2 * M_PI) * (int)(x / (2 * M_PI));
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    return x - x3/6.0f + x5/120.0f - x7/5040.0f;
}

static float cos(float x) {
    return sin(x + M_PI/2.0f);
}

void aa_put_pixel(float x, float y, uint32_t color) {
    int x0 = (int)x, y0 = (int)y;
    int x1 = x0 + 1, y1 = y0 + 1;
    float dx = x - x0, dy = y - y0;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    float a00 = (1.0f - dx) * (1.0f - dy);
    float a01 = (1.0f - dx) * dy;
    float a10 = dx * (1.0f - dy);
    float a11 = dx * dy;
    
    if (a00 > 0) put_pixel(x0, y0, makecolor(r*a00, g*a00, b*a00));
    if (a01 > 0) put_pixel(x0, y1, makecolor(r*a01, g*a01, b*a01));
    if (a10 > 0) put_pixel(x1, y0, makecolor(r*a10, g*a10, b*a10));
    if (a11 > 0) put_pixel(x1, y1, makecolor(r*a11, g*a11, b*a11));
}

void aa_line(float x0, float y0, float x1, float y1, uint32_t color) {
    float dx = ABS(x1 - x0), dy = ABS(y1 - y0);
    float sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    float err = dx - dy, e2, x2;
    float ed = dx + dy == 0 ? 1 : sqrt(dx*dx + dy*dy);
    
    for (;;) {
        float alpha = 255 * (ABS(err - dx + dy) / ed - 0.5 + 1);
        aa_put_pixel(x0, y0, (color & 0xFFFFFF) | ((uint32_t)alpha << 24));
        e2 = err; x2 = x0;
        if (2*e2 >= -dx) {
            if (x0 == x1) break;
            if (e2 + dy < ed) {
                alpha = 255 * (e2 + dy) / ed;
                aa_put_pixel(x0, y0 + sy, (color & 0xFFFFFF) | ((uint32_t)alpha << 24));
            }
            err -= dy; x0 += sx;
        }
        if (2*e2 <= dy) {
            if (y0 == y1) break;
            if (dx - e2 < ed) {
                alpha = 255 * (dx - e2) / ed;
                aa_put_pixel(x2 + sx, y0, (color & 0xFFFFFF) | ((uint32_t)alpha << 24));
            }
            err += dx; y0 += sy;
        }
    }
}

void aa_circle(float cx, float cy, float r, uint32_t color) {
    int x = r, y = 0;
    float radiusError = 1 - x;
    
    while (x >= y) {
        float dist = sqrt(x*x + y*y) - r;
        float alpha = 1.0f - smoothstep(0.0f, 1.0f, ABS(dist));
        
        aa_put_pixel(cx + x, cy + y, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx - x, cy + y, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx + x, cy - y, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx - x, cy - y, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx + y, cy + x, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx - y, cy + x, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx + y, cy - x, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        aa_put_pixel(cx - y, cy - x, (color & 0xFFFFFF) | (uint32_t)(alpha * 255) << 24);
        
        y++;
        if (radiusError < 0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}

void aa_rect(float x, float y, float w, float h, float border_thick, uint32_t fill_color, uint32_t border_color) {
    float outer_left = x;
    float outer_right = x + w;
    float outer_top = y;
    float outer_bottom = y + h;
    
    float inner_left = x + border_thick;
    float inner_right = x + w - border_thick;
    float inner_top = y + border_thick;
    float inner_bottom = y + h - border_thick;
    for (float py = y; py <= y + h; py += 0.5f) {
        for (float px = x; px <= x + w; px += 0.5f) {
            if (px >= inner_left && px <= inner_right && 
                py >= inner_top && py <= inner_bottom) {
                aa_put_pixel(px, py, fill_color);
                continue;
            }
            float dist_left = px - outer_left;
            float dist_right = outer_right - px;
            float dist_top = py - outer_top;
            float dist_bottom = outer_bottom - py;
            float min_dist = MIN(dist_left, MIN(dist_right, MIN(dist_top, dist_bottom)));
            if (min_dist <= border_thick) {
                float alpha = smoothstep(0.0f, 1.0f, min_dist / border_thick);
                uint8_t r = mix((border_color >> 16) & 0xFF, (fill_color >> 16) & 0xFF, alpha);
                uint8_t g = mix((border_color >> 8) & 0xFF, (fill_color >> 8) & 0xFF, alpha);
                uint8_t b = mix(border_color & 0xFF, fill_color & 0xFF, alpha);
                aa_put_pixel(px, py, makecolor(r, g, b));
            }
        }
    }
}

void aa_triangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) {
    float minx = MIN(x1, MIN(x2, x3));
    float maxx = MAX(x1, MAX(x2, x3));
    float miny = MIN(y1, MIN(y2, y3));
    float maxy = MAX(y1, MAX(y2, y3));
    
    float area = (y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3);
    
    for (float py = miny; py <= maxy; py += 0.5f) {
        for (float px = minx; px <= maxx; px += 0.5f) {
            float w1 = ((y2 - y3)*(px - x3) + (x3 - x2)*(py - y3)) / area;
            float w2 = ((y3 - y1)*(px - x3) + (x1 - x3)*(py - y3)) / area;
            float w3 = 1.0f - w1 - w2;
            
            if (w1 >= -0.01f && w2 >= -0.01f && w3 >= -0.01f) {
                float edge_dist = MIN(w1, MIN(w2, w3));
                float alpha = smoothstep(-0.01f, 0.01f, edge_dist);
                aa_put_pixel(px, py, (color & 0xFFFFFF) | ((uint32_t)(alpha * 255) << 24));
            }
        }
    }
}

void aa_semicircle(float cx, float cy, float r, int dir, uint32_t color) {
    float start_angle = dir == 0 ? 0 : M_PI;
    float end_angle = dir == 0 ? M_PI : 2*M_PI;
    
    for (float a = start_angle; a < end_angle; a += 0.01f) {
        float x = r * cos(a);
        float y = r * sin(a);
        for (float t = 0; t <= 1; t += 0.5f) {
            float px = cx + x * t;
            float py = cy + y * t;
            float dist = sqrt((px-cx)*(px-cx) + (py-cy)*(py-cy)) - r;
            float alpha = 1.0f - smoothstep(0.0f, 1.0f, ABS(dist));
            aa_put_pixel(px, py, (color & 0xFFFFFF) | ((uint32_t)(alpha * 255) << 24));
        }
    }
}