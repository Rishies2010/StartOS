#include "mem.h"
#include "../debug/log.h"
#include "../string.h"
#include "../limine.h"

typedef struct header {
    size_t size;
    int free;
    struct header* next;
} header_t;

#define HEADER_SIZE sizeof(header_t)

static header_t* heap_start = NULL;

// Limine memory map request
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

void init_kernel_heap(void) {
    struct limine_memmap_response *memmap = memmap_request.response;
    if (!memmap) {
        return;
    }
    uint64_t heap_size = 0x800000;
    uint64_t heap_phys = 0;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= heap_size) {
            heap_phys = entry->base;
            break;
        }
    }
    if (!heap_phys) {
        return;
    }
    uint64_t heap_virt = heap_phys + 0xffff800000000000;
    heap_start = (header_t*)heap_virt;
    heap_start->size = heap_size - HEADER_SIZE;
    heap_start->free = 1;
    heap_start->next = NULL;
    log("Kernel heap initialized.", 1);
}

void* kmalloc(size_t size) {
    if (!heap_start) {
        log("Heap not initialized!\n", 3);
        return NULL;
    }
    
    if (size == 0) {
        return NULL;
    }
    size = (size + 7) & ~7;
    
    header_t* curr = heap_start;
    while (curr) {
        if (curr->free && curr->size >= size) {
            // Check if we should split the block
            if (curr->size > size + HEADER_SIZE + 8) {
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
    
    log("kmalloc: no suitable block found\n", 3);
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    header_t* block = (header_t*)((uint8_t*)ptr - HEADER_SIZE);
    block->free = 1;
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }
    header_t* curr = heap_start;
    while (curr && curr->next != block) {
        curr = curr->next;
    }
    
    if (curr && curr->free) {
        curr->size += HEADER_SIZE + block->size;
        curr->next = block->next;
    }
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    header_t* block = (header_t*)((uint8_t*)ptr - HEADER_SIZE);
    if (block->size >= size) {
        return ptr;
    }
    void* new_mem = kmalloc(size);
    if (new_mem) {
        size_t copy_size = (block->size < size) ? block->size : size;
        for (size_t i = 0; i < copy_size; i++) {
            ((char*)new_mem)[i] = ((char*)ptr)[i];
        }
        kfree(ptr);
    }
    return new_mem;
}