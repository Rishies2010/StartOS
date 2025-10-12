#include "elf.h"
#include "../string.h"
#include "../debug/log.h"
#include "../../drv/disk/sfs.h"
#include "../../drv/vga.h"
#include "mem.h"
#include "../../drv/keyboard.h"
#include "../../drv/speaker.h"
#include "../../kernel/sched.h"

typedef struct
{
    int (*entry)(kernel_api_t *);
    kernel_api_t api;
    uint8_t *prog_mem;
    char name[64];
} elf_context_t;

static void elf_task_wrapper(void)
{
    asm volatile("cli");

    extern elf_context_t *g_current_elf_context;
    elf_context_t *ctx = g_current_elf_context;

    if (!ctx)
    {
        log("No context!", 3, 0);
        return;
    }

    log("Starting execution of %s", 4, 0, ctx->name);

    int ret = ctx->entry(&ctx->api);

    log("%s returned %d", 4, 0, ctx->name, ret);

    if (ctx->prog_mem)
    {
        kfree(ctx->prog_mem);
    }
    kfree(ctx);

    asm volatile("sti");
}

elf_context_t *g_current_elf_context = NULL;

int elf_exec(const char *filename)
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

    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
    {
        log("Invalid magic", 3, 0);
        kfree(elf_data);
        return -1;
    }

    if (ehdr->e_ident[4] != ELF_CLASS_64)
    {
        log("Not 64-bit", 3, 0);
        kfree(elf_data);
        return -1;
    }

    uint8_t *entry_bytes = elf_data + 24;
    log("Entry bytes at offset 24: %02x %02x %02x %02x %02x %02x %02x %02x", 1, 0,
        entry_bytes[0], entry_bytes[1], entry_bytes[2], entry_bytes[3],
        entry_bytes[4], entry_bytes[5], entry_bytes[6], entry_bytes[7]);

    log("Valid 64-bit ELF, entry: 0x%lx", 1, 0, ehdr->e_entry);

    uint64_t min_addr = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_addr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD)
        {
            log("Segment %d: vaddr=0x%lx, memsz=0x%lx", 1, 0, i, phdr->p_vaddr, phdr->p_memsz);
            if (phdr->p_vaddr < min_addr)
                min_addr = phdr->p_vaddr;
            if (phdr->p_vaddr + phdr->p_memsz > max_addr)
            {
                max_addr = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

    log("Address range: 0x%lx to 0x%lx", 1, 0, min_addr, max_addr);

    uint64_t total_size = max_addr - min_addr;
    log("Need 0x%lx bytes", 1, 0, total_size);

    uint8_t *prog = (uint8_t *)kmalloc(total_size);
    if (!prog)
    {
        log("Failed to allocate program memory", 3, 0);
        kfree(elf_data);
        return -1;
    }

    memset(prog, 0, total_size);
    log("Allocated at 0x%lx", 1, 0, (uint64_t)prog);

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD)
        {
            log("Loading segment %d, vaddr=0x%lx, size=0x%lx", 1, 0,
                i, phdr->p_vaddr, phdr->p_memsz);

            uint64_t offset = phdr->p_vaddr - min_addr;
            uint8_t *dest = prog + offset;
            uint8_t *src = elf_data + phdr->p_offset;

            for (uint64_t j = 0; j < phdr->p_filesz; j++)
            {
                dest[j] = src[j];
            }
        }
    }

    uint64_t elf_entry_point = ehdr->e_entry;
    uint16_t elf_phnum = ehdr->e_phnum;

    kfree(elf_data);

    elf_context_t *ctx = (elf_context_t *)kmalloc(sizeof(elf_context_t));
    if (!ctx)
    {
        log("Failed to allocate context", 3, 0);
        kfree(prog);
        return -1;
    }

    ctx->api.prints = prints;
    ctx->api.printc = printc;
    ctx->api.setcolor = setcolor;
    ctx->api.log_internal = log_internal;
    ctx->api.kmalloc = kmalloc;
    ctx->api.kfree = kfree;
    ctx->api.put_pixel = put_pixel;
    ctx->api.read_line = read_line;

    uint64_t final_entry = (uint64_t)prog + elf_entry_point - min_addr;

    ctx->entry = (int (*)(kernel_api_t *))final_entry;
    ctx->prog_mem = prog;

    int i = 0;
    while (filename[i] && i < 63)
    {
        ctx->name[i] = filename[i];
        i++;
    }
    ctx->name[i] = '\0';

    log("Creating task for %s at 0x%lx", 4, 0, ctx->name, (uint64_t)ctx->entry);

    g_current_elf_context = ctx;

    task_t *task = task_create_user(elf_task_wrapper, ctx->name);
    if (!task)
    {
        log("Failed to create task", 3, 0);
        kfree(prog);
        kfree(ctx);
        g_current_elf_context = NULL;
        return -1;
    }

    log("Task created (PID %d)", 4, 0, task->pid);

    return 0;
}