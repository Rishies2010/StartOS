#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char name[28];
    uint32_t size;
    uint32_t start_block;
    uint32_t position;                  
    uint8_t entry_index;                
    uint8_t is_open;
} sfs_file_t;

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
    void (*f_open)(const char* filename, sfs_file_t* file);
    void (*f_read)(sfs_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
    void (*f_write)(sfs_file_t* file, const void* buffer, uint32_t size);
    void (*f_close)(sfs_file_t* file);
    void (*f_mk)(const char* filename, uint32_t size);
    void (*f_rm)(const char* filename);
    void (*sleep)(uint32_t time);
} kernel_api_t;

#define log(api, fmt, level, vis, ...) \
    api->log_internal(__FILE__, __LINE__, fmt, level, vis, ##__VA_ARGS__)

int main(kernel_api_t *api)
{
    api->prints("\n Hello from ELF !\n");
    sfs_file_t file;
    api->f_open("test.txt", &file);
    char buffer[100];
    uint32_t read;
    api->f_read(&file, buffer, 100, &read);
    buffer[read] = '\0';
    log(api, "\nRead: %s", 4, 1, buffer);
    api->f_close(&file);
    return 123;
}