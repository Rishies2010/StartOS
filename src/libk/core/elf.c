#include "elf.h"
#include "../string.h"
#include "../debug/log.h"
#include "../../drv/vga.h"
#include "mem.h"
#include "../../drv/keyboard.h"
#include "../../kernel/sched.h"
#include "../../drv/mouse.h"
#include "../../drv/speaker.h"
#include "../../drv/net/e1000.h"

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

    uint8_t *prog = (uint8_t *)kmalloc(total_size);
    if (!prog)
    {
        log("Failed to allocate program memory", 3, 0);
        kfree(elf_data);
        return -1;
    }

    memset(prog, 0, total_size);

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(elf_data + ehdr->e_phoff + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD)
        {
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
    ctx->api.get_key = get_key;
    ctx->api.wait_for_key = wait_for_key;
    ctx->api.is_alt_pressed = is_alt_pressed;
    ctx->api.is_shift_pressed = is_shift_pressed;
    ctx->api.is_caps_lock_on = is_caps_lock_on;
    ctx->api.is_ctrl_pressed = is_ctrl_pressed;
    ctx->api.is_key_pressed = is_key_pressed;
    ctx->api.f_format      = sfs_format;
    ctx->api.f_init        = sfs_init;
    ctx->api.f_mkdir       = sfs_mkdir;
    ctx->api.f_rmdir       = sfs_rmdir;
    ctx->api.f_chdir       = sfs_chdir;
    ctx->api.f_get_cwd     = sfs_get_cwd;
    ctx->api.f_create      = sfs_create;
    ctx->api.f_open        = sfs_open;
    ctx->api.f_read        = sfs_read;
    ctx->api.f_write       = sfs_write;
    ctx->api.f_close       = sfs_close;
    ctx->api.f_rm          = sfs_delete;
    ctx->api.f_mk          = sfs_create;
    ctx->api.f_seek        = sfs_seek;
    ctx->api.f_list        = sfs_list;
    ctx->api.f_print_stats = sfs_print_stats;
    ctx->api.f_unmount     = sfs_unmount;
    ctx->api.sleep = sleep;
    ctx->api.sched_yield = sched_yield;
    ctx->api.strlen     = strlen;
    ctx->api.strcpy     = strcpy;
    ctx->api.strncpy    = strncpy;
    ctx->api.strcat     = strcat;
    ctx->api.strncat    = strncat;
    ctx->api.strcmp     = strcmp;
    ctx->api.strncmp    = strncmp;
    ctx->api.strchr     = strchr;
    ctx->api.strrchr    = strrchr;
    ctx->api.strspn     = strspn;
    ctx->api.strcspn    = strcspn;
    ctx->api.strstr     = strstr;
    ctx->api.strtok     = strtok;
    ctx->api.strtok_r   = strtok_r;
    ctx->api.memset     = memset;
    ctx->api.memcpy     = memcpy;
    ctx->api.memmove    = memmove;
    ctx->api.memcmp     = memcmp;
    ctx->api.memchr     = memchr;
    ctx->api.atoi       = atoi;
    ctx->api.itoa       = itoa;
    ctx->api.itoa_hex   = itoa_hex;
    ctx->api.vsnprintf  = vsnprintf;
    ctx->api.snprintf   = snprintf;
    ctx->api.toLower    = toLower;
    ctx->api.toUpper    = toUpper;
    ctx->api.get_pixel_at = get_pixel_at;
    ctx->api.exec = elf_exec;
    ctx->api.mouse_button = mouse_button;
    ctx->api.mouse_moved = mouse_moved;
    ctx->api.mouse_set_pos = mouse_set_pos;
    ctx->api.mouse_x = mouse_x;
    ctx->api.mouse_y = mouse_y;
    ctx->api.clr = clr;
    ctx->api.speaker_note = speaker_note;
    ctx->api.speaker_pause = speaker_pause;
    ctx->api.speaker_play = speaker_play;
    ctx->api.rtc_calculate_uptime = rtc_calculate_uptime;
    ctx->api.rtc_get_time = rtc_get_time;
    ctx->api.rtc_boottime = rtc_boottime;
    ctx->api.net_send_packet        = e1000_send_packet;
    ctx->api.net_receive_packet     = e1000_receive_packet;
    ctx->api.net_get_mac_address    = e1000_get_mac_address;
    ctx->api.net_link_up            = e1000_link_up;
    ctx->api.net_enable_interrupts  = e1000_enable_interrupts;
    ctx->api.net_disable_interrupts = e1000_disable_interrupts;
    ctx->api.net_get_interrupt_status = e1000_get_interrupt_status;
    ctx->api.net_handle_interrupt   = e1000_handle_interrupt;


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