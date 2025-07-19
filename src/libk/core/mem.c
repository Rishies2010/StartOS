#include "mem.h"
#include "../debug/log.h"
#include "../debug/serial.h"
#include "../string.h"
#include "../limine.h"

static uint8_t* pmm_bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;
static uint64_t bitmap_size = 0;

static uint64_t memory_base = 0;
static uint64_t memory_top = 0;

typedef struct header {
    size_t size;
    int free;
    struct header* next;
} header_t;

#define HEADER_SIZE sizeof(header_t)
static header_t* heap_start = NULL;

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static void set_bit(uint64_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static void clear_bit(uint64_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static int test_bit(uint64_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

static uint64_t find_free_pages(size_t count) {
    if (count == 1) {
        for (uint64_t i = 0; i < total_pages; i++) {
            if (!test_bit(i)) {
                return i;
            }
        }
    } else {
        for (uint64_t i = 0; i <= total_pages - count; i++) {
            int found = 1;
            for (size_t j = 0; j < count; j++) {
                if (test_bit(i + j)) {
                    found = 0;
                    break;
                }
            }
            if (found) {
                return i;
            }
        }
    }
    return UINT64_MAX;
}

void init_pmm(void) {
    struct limine_memmap_response *memmap = memmap_request.response;
    if (!memmap) {
        serial_write_string("-[ERROR] - PMM: Failed to get memory map!\n");
        return;
    }
    memory_base = UINT64_MAX;
    memory_top = 0;
    
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            if (entry->base < memory_base) {
                memory_base = entry->base;
            }
            if (entry->base + entry->length > memory_top) {
                memory_top = entry->base + entry->length;}}}
    total_pages = (memory_top - memory_base) / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;
    uint64_t bitmap_phys = 0;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap_phys = entry->base;
            break;
        }}    
    if (!bitmap_phys) {
        serial_write_string("-[ERROR] - PMM: Can't find space for bitmap!\n");
        return;
    }
    pmm_bitmap = (uint8_t*)(bitmap_phys + 0xffff800000000000);
    for (uint64_t i = 0; i < bitmap_size; i++) {
        pmm_bitmap[i] = 0xFF;
    }
    used_pages = total_pages;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = (entry->base - memory_base) / PAGE_SIZE;
            uint64_t page_count = entry->length / PAGE_SIZE;
            
            for (uint64_t j = 0; j < page_count; j++) {
                if (start_page + j < total_pages) {
                    clear_bit(start_page + j);
                    used_pages--;
                }
            }
        }
    }
    uint64_t bitmap_start_page = (bitmap_phys - memory_base) / PAGE_SIZE;
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        if (bitmap_start_page + i < total_pages && !test_bit(bitmap_start_page + i)) {
            set_bit(bitmap_start_page + i);
            used_pages++;
        }
    }
    serial_write_string("-[PASS] - PMM Initialized successfully\n");
}

uint64_t alloc_page(void) {
    return alloc_pages(1);
}

uint64_t alloc_pages(size_t count) {
    if (!pmm_bitmap || count == 0) {
        return 0;
    }
    
    uint64_t page_idx = find_free_pages(count);
    if (page_idx == UINT64_MAX) {
        log("PMM: Out of memory!", 3, 1);
        return 0;
    }
    for (size_t i = 0; i < count; i++) {
        set_bit(page_idx + i);
        used_pages++;
    }
    
    return memory_base + (page_idx * PAGE_SIZE);
}

void free_page(uint64_t addr) {
    free_pages(addr, 1);
}

void free_pages(uint64_t addr, size_t count) {
    if (!pmm_bitmap || addr < memory_base || addr >= memory_top) {
        return;
    }
    
    uint64_t page_idx = (addr - memory_base) / PAGE_SIZE;
    
    for (size_t i = 0; i < count; i++) {
        if (page_idx + i < total_pages && test_bit(page_idx + i)) {
            clear_bit(page_idx + i);
            used_pages--;
        }
    }
}

uint64_t get_total_memory(void) {
    return total_pages * PAGE_SIZE;
}

uint64_t get_free_memory(void) {
    return (total_pages - used_pages) * PAGE_SIZE;
}

void init_kernel_heap(void) {
    uint64_t heap_pages = 2048;
    uint64_t heap_phys = alloc_pages(heap_pages);
    if (!heap_phys) {
        log("Failed to allocate heap pages!", 3, 1);
        return;}    
    uint64_t heap_virt = heap_phys + 0xffff800000000000;
    heap_start = (header_t*)heap_virt;
    heap_start->size = (heap_pages * PAGE_SIZE) - HEADER_SIZE;
    heap_start->free = 1;
    heap_start->next = NULL;
    log("Kernel heap initialized.", 4, 0);
}

void print_mem_info(int vis){
    log("\n -> Memory Statistics:\n\n - Total Memory:\n   %lu MBs.\n\n - Free Memory:\n   %lu MBs.\n\n - Used Memory:\n   %lu MBs.\n", 1, vis, get_total_memory()/1048576, get_free_memory()/1048576, (get_total_memory() - get_free_memory())/1048576);
}

void* kmalloc(size_t size) {
    if (!heap_start) {
        log("Heap not initialized!", 3, 1);
        return NULL;
    }
    if (size == 0) {
        return NULL;
    }
    size = (size + 7) & ~7;
    
    header_t* curr = heap_start;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + HEADER_SIZE + 8) {
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
    log("kmalloc: no suitable block found", 3, 1);
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