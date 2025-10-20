#include "elf.h"
#include "../string.h"
#include "../debug/log.h"
#include "../../drv/vga.h"
#include "mem.h"

typedef struct
{
    int (*entry)(int argc, char **argv);
    uint64_t prog_virt;      // Just store virt address
    uint64_t prog_phys;      // Just store phys address  
    uint64_t prog_size;      // Just store size
    char name[64];
    int argc;
    char **argv;
} elf_context_t;

static void elf_task_wrapper(void)
{
    extern elf_context_t *g_current_elf_context;
    elf_context_t *ctx = g_current_elf_context;

    if (!ctx)
    {
        log("No context!", 3, 0);
        return;
    }

    log("Starting execution of %s", 4, 0, ctx->name);
    
    int ret = ctx->entry(ctx->argc, ctx->argv);
    
    log("%s returned %d", 4, 0, ctx->name, ret);

    // Cleanup
    if (ctx->argv)
    {
        for (int i = 0; i < ctx->argc; i++)
            if (ctx->argv[i]) kfree(ctx->argv[i]);
        kfree(ctx->argv);
    }
    
    // Free program memory
    if (ctx->prog_virt)
    {
        page_table_t *pml4 = get_kernel_pml4();
        size_t pages = (ctx->prog_size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (size_t i = 0; i < pages; i++)
            unmap_page(pml4, ctx->prog_virt + (i * PAGE_SIZE));
        free_pages(ctx->prog_phys, pages);
    }
    
    kfree(ctx);
}

elf_context_t *g_current_elf_context = NULL;

int elf_exec(const char *filename, int argc, char **argv)
{
    log("Loading %s", 1, 0, filename);
    
    sfs_file_t file;
    if (sfs_open(filename, &file) != SFS_OK)
    {
        log("Failed to open file", 3, 0);
        return -1;
    }

    uint8_t *elf_data = (uint8_t *)kmalloc(file.size);
    if (!elf_data)
    {
        log("Out of memory", 3, 0);
        sfs_close(&file);
        return -1;
    }

    uint32_t bytes_read;
    if (sfs_read(&file, elf_data, file.size, &bytes_read) != SFS_OK)
    {
        log("Read failed", 3, 0);
        kfree(elf_data);
        sfs_close(&file);
        return -1;
    }
    sfs_close(&file);

    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)elf_data;

    // Validate ELF
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
    {
        log("Invalid ELF magic", 3, 0);
        kfree(elf_data);
        return -1;
    }

    if (ehdr->e_ident[4] != ELF_CLASS_64)
    {
        log("Not 64-bit ELF", 3, 0);
        kfree(elf_data);
        return -1;
    }

    // Find load address range
    uint64_t min_addr = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_addr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);
        if (phdr->p_type == PT_LOAD)
        {
            if (phdr->p_vaddr < min_addr) min_addr = phdr->p_vaddr;
            if (phdr->p_vaddr + phdr->p_memsz > max_addr) max_addr = phdr->p_vaddr + phdr->p_memsz;
        }
    }

    uint64_t total_size = max_addr - min_addr;
    log("ELF: min=0x%lx max=0x%lx size=0x%lx entry=0x%lx", 1, 0, 
        min_addr, max_addr, total_size, ehdr->e_entry);

    // Allocate memory for program
    size_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t prog_phys = alloc_pages(pages);
    if (!prog_phys)
    {
        log("Failed to allocate program pages", 3, 0);
        kfree(elf_data);
        return -1;
    }

    // Use kernel addresses (ring 0 for now)
    uint64_t prog_virt = prog_phys + KERNEL_VIRT_OFFSET;
    
    // Zero it
    memset((void*)prog_virt, 0, total_size);

    // Load segments
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);
        if (phdr->p_type == PT_LOAD)
        {
            uint64_t offset = phdr->p_vaddr - min_addr;
            uint8_t *dest = (uint8_t*)(prog_virt + offset);
            uint8_t *src = elf_data + phdr->p_offset;
            memcpy(dest, src, phdr->p_filesz);
        }
    }

    kfree(elf_data);

    // Create context
    elf_context_t *ctx = (elf_context_t *)kmalloc(sizeof(elf_context_t));
    if (!ctx)
    {
        log("Failed to allocate context", 3, 0);
        free_pages(prog_phys, pages);
        return -1;
    }

    // Calculate entry point
    uint64_t final_entry = prog_virt + ehdr->e_entry - min_addr;
    log("Entry point: 0x%lx", 1, 0, final_entry);
    
    ctx->entry = (int (*)(int, char **))final_entry;
    ctx->prog_virt = prog_virt;
    ctx->prog_phys = prog_phys;
    ctx->prog_size = total_size;

    // Copy name
    strncpy(ctx->name, filename, 63);
    ctx->name[63] = '\0';

    // Copy argc/argv
    ctx->argc = argc;
    if (argc > 0 && argv)
    {
        ctx->argv = (char **)kmalloc(sizeof(char *) * argc);
        if (!ctx->argv)
        {
            log("Failed to allocate argv", 3, 0);
            free_pages(prog_phys, pages);
            kfree(ctx);
            return -1;
        }

        for (int j = 0; j < argc; j++)
        {
            if (argv[j])
            {
                size_t len = strlen(argv[j]) + 1;
                ctx->argv[j] = (char *)kmalloc(len);
                if (ctx->argv[j])
                    strcpy(ctx->argv[j], argv[j]);
                else
                {
                    for (int k = 0; k < j; k++) kfree(ctx->argv[k]);
                    kfree(ctx->argv);
                    free_pages(prog_phys, pages);
                    kfree(ctx);
                    return -1;
                }
            }
            else
                ctx->argv[j] = NULL;
        }
    }
    else
        ctx->argv = NULL;

    g_current_elf_context = ctx;

    // Create kernel task (ring 0 for now)
    task_t *task = task_create(elf_task_wrapper, ctx->name);
    if (!task)
    {
        log("Failed to create task", 3, 0);
        if (ctx->argv)
        {
            for (int j = 0; j < argc; j++) kfree(ctx->argv[j]);
            kfree(ctx->argv);
        }
        free_pages(prog_phys, pages);
        kfree(ctx);
        g_current_elf_context = NULL;
        return -1;
    }

    log("Task created (PID %d)", 4, 0, task->pid);
    return 0;
}