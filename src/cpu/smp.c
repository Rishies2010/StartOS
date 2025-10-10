#include "smp.h"
#include "../libk/debug/log.h"
#include "../drv/local_apic.h"
#include "../libk/string.h"
#include "sse_fpu.h"
#include "../libk/limine.h"
#include "gdt.h"
#include "idt.h"
#include "id/core.h"
#include "../drv/vga.h"

#define STACK_SIZE 4096

static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

volatile uint32_t g_activeCpuCount = 1;
static uint8_t ap_stacks[MAX_CPU_COUNT][STACK_SIZE] __attribute__((aligned(16)));

void ap_entry(struct limine_smp_info *info) {
    asm volatile("mov %0, %%rsp" : : "r" (ap_stacks[info->processor_id] + STACK_SIZE) : "memory");
    init_gdt();
    init_idt();
    enable_sse_and_fpu();
    LocalApicInit();
    __atomic_add_fetch(&g_activeCpuCount, 1, __ATOMIC_SEQ_CST);
    ap_main();
    while(1);
}

void init_smp() {
    struct limine_smp_response *smp = smp_request.response;
    if (smp == NULL || smp->cpu_count < 2) {
        clr();
        log("\n\nThis system cannot run StartOS\n\n - This system does not meet the requirement of minimum 2 CPUs.\n\nConsider upgrading your CPU or computer.\nIncrease CPUs available if on a VM.\n ", 3, 1);
        log(" Halting StartOS.", 2, 1);
        __asm__ __volatile__("cli");
        for(;;)__asm__ __volatile__("hlt");
    }
    log("Bootstrap Processor ID: %d, Total CPUs: %d", 1, 0,
        smp->bsp_lapic_id, smp->cpu_count);
    for (size_t i = 0; i < smp->cpu_count; i++) {
        if (smp->cpus[i]->lapic_id != smp->bsp_lapic_id) {
            log("Starting CPU %lu (LAPIC ID %d)", 1, 0,
                i, smp->cpus[i]->lapic_id);
            smp->cpus[i]->goto_address = ap_entry;
        }
    }
    while (g_activeCpuCount < smp->cpu_count) {
        asm volatile("pause");
    }
    log("All %d CPUs online", 4, 0, smp->cpu_count);
}