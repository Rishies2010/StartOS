#include <stddef.h>
#include <stdint.h>

void init_kernel_heap(void);

void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);