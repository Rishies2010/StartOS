#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))

// PMM functions
void init_pmm(void);
uint64_t alloc_page(void);
void free_page(uint64_t addr);
uint64_t alloc_pages(size_t count);
void free_pages(uint64_t addr, size_t count);

// Heap functions (built on top of PMM)
void init_kernel_heap(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

// Utility functions
uint64_t get_total_memory(void);
uint64_t get_free_memory(void);

#endif