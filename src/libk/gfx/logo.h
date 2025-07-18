#ifndef LOGO_H
#define LOGO_H

#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bytes_per_pixel;
    const char* comment;
    const uint8_t* pixel_data;
} startos_img_t;

extern const startos_img_t startos;

#endif
