#include "tga.h"
#include "../../drv/vga.h"
#include "../../drv/disk/sfs.h"
#include "../string.h"
#include "../debug/log.h"
#include "../core/mem.h"

typedef struct
{
    uint8_t id_length;
    uint8_t color_map_type;
    uint8_t image_type;
    uint16_t color_map_origin;
    uint16_t color_map_length;
    uint8_t color_map_depth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t descriptor;
} __attribute__((packed)) tga_header_t;

void plotimg(const char *filename, int x, int y)
{
    sfs_file_t file;
    if (sfs_open(filename, &file) != SFS_OK)
    {
        log("Failed to open TGA file", 3, 0);
        return;
    }

    tga_header_t header;
    uint32_t bytes_read;

    if (sfs_read(&file, &header, sizeof(header), &bytes_read) != SFS_OK || bytes_read != sizeof(header))
    {
        log("Failed to read TGA header", 3, 0);
        sfs_close(&file);
        return;
    }

    if (header.image_type != 2 || (header.bpp != 32 && header.bpp != 24))
    {
        log("Unsupported TGA format", 3, 0);
        sfs_close(&file);
        return;
    }

    uint32_t skip_bytes = header.id_length + (header.color_map_type ? (header.color_map_length * (header.color_map_depth >> 3)) : 0);
    uint32_t pixel_size = header.bpp >> 3;
    uint32_t data_size = header.width * header.height * pixel_size;

    uint8_t *buffer = kmalloc(skip_bytes + data_size);
    if (!buffer)
    {
        sfs_close(&file);
        return;
    }

    if (sfs_read(&file, buffer, skip_bytes + data_size, &bytes_read) != SFS_OK)
    {
        kfree(buffer);
        sfs_close(&file);
        return;
    }
    sfs_close(&file);

    uint8_t *pixel_data = buffer + skip_bytes;

    int32_t start_x = (x < 0) ? 0 : x;
    int32_t start_y = (y < 0) ? 0 : y;
    int32_t src_start_x = (x < 0) ? -x : 0;
    int32_t src_start_y = (y < 0) ? -y : 0;
    int32_t end_x = (x + header.width > framebuffer_width) ? framebuffer_width : (x + header.width);
    int32_t end_y = (y + header.height > framebuffer_height) ? framebuffer_height : (y + header.height);

    if (start_x >= end_x || start_y >= end_y)
    {
        kfree(buffer);
        log("Image out of bounds", 3, 0);
        return;
    }

    uint32_t *fb = (uint32_t *)framebuffer_addr;

    if (pixel_size == 4)
    {

        uint32_t *src = (uint32_t *)pixel_data;
        uint32_t copy_width = end_x - start_x;
        for (int32_t py = start_y; py < end_y; py++)
        {
            memcpy(fb + py * framebuffer_width + start_x,
                   src + (py - y + src_start_y) * header.width + src_start_x,
                   copy_width << 2);
        }
    }
    else
    {

        uint8_t *src = pixel_data;
        for (int32_t py = start_y; py < end_y; py++)
        {
            uint32_t *dst = fb + py * framebuffer_width + start_x;
            uint8_t *s = src + ((py - y + src_start_y) * header.width + src_start_x) * 3;
            uint32_t w = end_x - start_x;

            while (w >= 4)
            {
                dst[0] = s[0] | (s[1] << 8) | (s[2] << 16);
                dst[1] = s[3] | (s[4] << 8) | (s[5] << 16);
                dst[2] = s[6] | (s[7] << 8) | (s[8] << 16);
                dst[3] = s[9] | (s[10] << 8) | (s[11] << 16);
                dst += 4;
                s += 12;
                w -= 4;
            }
            while (w--)
            {
                *dst++ = s[0] | (s[1] << 8) | (s[2] << 16);
                s += 3;
            }
        }
    }

    kfree(buffer);
}