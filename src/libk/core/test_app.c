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
    api->setcolor(0xFF00FF, 0x000000);
    api->prints(" I am an ELF file ! Running using kernel API !\n");

    log(api, "Hello from ELF using log()!", 4, 1);

    void *mem = api->kmalloc(1024);
    if (mem)
    {
        log(api, "Successfully allocated 1024 bytes at 0x%lx", 4, 1, (uint64_t)mem);
        api->kfree(mem);
        api->prints(" Memory freed!\n");
    }

    api->prints(" Calculating 1+2+...+10 = ");
    int sum = 0;
    for (int i = 1; i <= 10; i++)
    {
        sum += i;
    }

    api->printc('0' + (sum / 10));
    api->printc('0' + (sum % 10));
    api->printc('\n');

    api->setcolor(0xFFFFFF, 0x000000);

    api->prints("\n All tests passed! After screen clear, press enter to return...\n");
    for(int i = 0; i < 599999999; i++);
    for(int i = 0; i < 1280; i++)for(int j = 0; j < 800; j++)api->put_pixel(i, j, 0xffffff);
    char buffer[64];
    asm volatile("sti");
    api->read_line(buffer, 64, true);
    asm volatile("cli");

    return 123;
}