#pragma once
#include <stddef.h>
#include <stdint.h>

void init_kernel_heap(uint32_t heap_start, uint32_t heap_size);

void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
