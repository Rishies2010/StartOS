#include "mem.h"
#include "../logic/log/logging.h"

typedef struct header {
    size_t size;
    int free;
    struct header* next;
} header_t;

#define HEADER_SIZE sizeof(header_t)

static header_t* heap_start = NULL;

void init_kernel_heap(uint32_t start, uint32_t size) {
    heap_start = (header_t*)start;
    heap_start->size = size - HEADER_SIZE;
    heap_start->free = 1;
    heap_start->next = NULL;
}

void* kmalloc(size_t size) {
    header_t* curr = heap_start;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + HEADER_SIZE) {
                // Split block
                header_t* next = (header_t*)((uint8_t*)curr + HEADER_SIZE + size);
                next->size = curr->size - size - HEADER_SIZE;
                next->free = 1;
                next->next = curr->next;

                curr->size = size;
                curr->next = next;
            }
            curr->free = 0;
            return (void*)((uint8_t*)curr + HEADER_SIZE);
        }
        curr = curr->next;
    }
    log("Out Of Memory !", 3, 1);
    return NULL; // No memory
}

void kfree(void* ptr) {
    if (!ptr) return;
    header_t* block = (header_t*)((uint8_t*)ptr - HEADER_SIZE);
    block->free = 1;

    // Coalesce with next block if it's free
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);

    header_t* block = (header_t*)((uint8_t*)ptr - HEADER_SIZE);
    if (block->size >= size) return ptr;

    void* new_mem = kmalloc(size);
    if (new_mem) {
        for (size_t i = 0; i < block->size; i++) {
            ((char*)new_mem)[i] = ((char*)ptr)[i];
        }
        kfree(ptr);
    }
    return new_mem;
}
