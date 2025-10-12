#include "elf.h"
#include "../string.h"
#include "../debug/log.h"
#include "../../drv/disk/sfs.h"
#include "../../drv/vga.h"
#include "mem.h"

extern void prints(const char *str);
extern void printc(char c);
extern void setcolor(uint8_t fg, uint8_t bg);
extern void log_internal(const char *file, int line, const char *fmt, int level, int visibility, ...);

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

    log("Valid 64-bit ELF, entry: 0x%lx", 1, 0, ehdr->e_entry);

    uint64_t min_addr = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_addr = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD)
        {
            if (phdr->p_vaddr < min_addr)
                min_addr = phdr->p_vaddr;
            if (phdr->p_vaddr + phdr->p_memsz > max_addr)
            {
                max_addr = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

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

    kernel_api_t api;
    api.prints = prints;
    api.printc = printc;
    api.setcolor = setcolor;
    api.log_internal = log_internal;
    api.kmalloc = kmalloc;
    api.kfree = kfree;

    log("API struct at 0x%lx", 1, 0, (uint64_t)&api);
    log("prints at 0x%lx", 1, 0, (uint64_t)prints);

    uint64_t entry_offset = ehdr->e_entry - min_addr;
    int (*entry)(kernel_api_t *) = (int (*)(kernel_api_t *))(prog + entry_offset);

    log("Jumping to 0x%lx with API", 4, 0, (uint64_t)entry);

    kfree(elf_data);

    int ret = entry(&api);

    log("Program returned %d", 4, 0, ret);

    kfree(prog);
    return ret;
}