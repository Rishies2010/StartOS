#ifndef AA_SHAPES_H
#define AA_SHAPES_H

#include <stdint.h>

void aa_put_pixel(float x, float y, uint32_t color);
void aa_line(float x0, float y0, float x1, float y1, uint32_t color);
void aa_circle(float cx, float cy, float r, uint32_t color);
void aa_rect(float x, float y, float w, float h, float border_thick, uint32_t fill_color, uint32_t border_color);
void aa_triangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color);
void aa_semicircle(float cx, float cy, float r, int dir, uint32_t color);

#endif