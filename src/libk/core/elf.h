#ifndef ELF_H
#define ELF_H

#include <stdint.h>

// ELF Magic
#define ELF_MAGIC 0x464C457F

// ELF Classes
#define ELF_CLASS_64 2

// ELF Types
#define ET_EXEC 2

// ELF Machine
#define EM_X86_64 62

// Program Header Types
#define PT_LOAD 1

// Program Header Flags
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

// ELF Header
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf64_ehdr_t;

// Program Header
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

// Exec function
int elf_exec(const char* filename);

#endif