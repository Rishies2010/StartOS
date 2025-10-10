#include "tga.h"
#include "../../drv/vga.h"
#include "../../drv/disk/sfs.h"
#include "../string.h"
#include "../debug/log.h"
#include "../core/mem.h"

typedef struct {
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

void plotimg(const char *filename, int x, int y) {
    sfs_file_t file;
    if(sfs_open(filename, &file) != SFS_OK) {
        log("Failed to open TGA file", 3, 0);
        return;
    }

    tga_header_t header;
    uint32_t bytes_read;
    if(sfs_read(&file, &header, sizeof(header), &bytes_read) != SFS_OK || bytes_read != sizeof(header)) {
        log("Failed to read TGA header", 3, 0);
        sfs_close(&file);
        return;
    }

    if(header.image_type != 2) {
        log("Unsupported TGA format", 3, 0);
        sfs_close(&file);
        return;
    }

    if(header.bpp != 32 && header.bpp != 24) {
        log("Unsupported TGA BPP", 3, 0);
        sfs_close(&file);
        return;
    }

    sfs_seek(&file, sizeof(header) + header.id_length + (header.color_map_type ? (header.color_map_length * (header.color_map_depth / 8)) : 0));

    uint32_t pixel_count = header.width * header.height;
    uint32_t pixel_size = header.bpp / 8;
    uint8_t *pixel_data = kmalloc(pixel_count * pixel_size);
    
    if(!pixel_data) {
        log("Failed to allocate pixel buffer", 3, 0);
        sfs_close(&file);
        return;
    }

    if(sfs_read(&file, pixel_data, pixel_count * pixel_size, &bytes_read) != SFS_OK || bytes_read != pixel_count * pixel_size) {
        log("Failed to read pixel data", 3, 0);
        kfree(pixel_data);
        sfs_close(&file);
        return;
    }

    for(uint32_t py = 0; py < header.height; py++) {
        for(uint32_t px = 0; px < header.width; px++) {
            uint32_t screen_x = x + px;
            uint32_t screen_y = y + py;
            
            if(screen_x >= framebuffer_width || screen_y >= framebuffer_height) continue;
            
            uint32_t pixel_index = (py * header.width + px) * pixel_size;
            uint32_t color;
            
            if(header.bpp == 32) {
                uint32_t *pixel = (uint32_t*)(pixel_data + pixel_index);
                color = *pixel;
            } else {
                uint8_t *pixel = pixel_data + pixel_index;
                color = (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
            }
            
            put_pixel(screen_x, screen_y, color);
        }
    }

    kfree(pixel_data);
    sfs_close(&file);
}