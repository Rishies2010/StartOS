#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    void (*prints)(const char *str);
    void (*printc)(char c);
    void (*setcolor)(uint32_t fg, uint32_t bg);
    void (*log_internal)(const char *file, int line, const char *fmt, int level, int visibility, ...);
    void *(*kmalloc)(uint64_t size);
    void (*kfree)(void *ptr);
    void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);
    void (*read_line)(char *buffer, size_t max_size, bool print);
} kernel_api_t;

#define log(api, fmt, level, vis, ...) \
    api->log_internal(__FILE__, __LINE__, fmt, level, vis, ##__VA_ARGS__)

int main(kernel_api_t *api)
{
    api->prints("\n Hello from ELF !\n");
    return 123;
}